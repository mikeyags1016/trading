#include "order_book.hpp"

#include <cmath>
#include <utility>

namespace trading {

bool OrderBook::InstrumentLess::operator()(
		const Instrument& left,
		const Instrument& right) const {
	if (left.symbol != right.symbol) {
		return left.symbol < right.symbol;
	}
	if (left.exchange != right.exchange) {
		return left.exchange < right.exchange;
	}
	return left.currency < right.currency;
}

bool OrderBook::is_stale(const BookState& book, long long timestamp_ns) {
	return book.timestamp_ns > 0
		&& timestamp_ns > 0
		&& timestamp_ns < book.timestamp_ns;
}

void OrderBook::update_timestamp(BookState& book, long long timestamp_ns) {
	if (timestamp_ns > book.timestamp_ns) {
		book.timestamp_ns = timestamp_ns;
	}
}

bool OrderBook::update_level(
		const Instrument& instrument,
		OrderSide side,
		double price,
		int quantity,
		long long timestamp_ns) {
	if (!is_valid_instrument(instrument)
			|| !std::isfinite(price)
			|| price <= 0.0
			|| quantity <= 0
			|| timestamp_ns < 0
			|| (side != OrderSide::Buy && side != OrderSide::Sell)) {
		return false;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	BookState& book = books_[instrument];
	if (is_stale(book, timestamp_ns)) {
		return false;
	}

	if (side == OrderSide::Buy) {
		book.bids[price] = quantity;
	} else {
		book.asks[price] = quantity;
	}
	update_timestamp(book, timestamp_ns);
	return true;
}

bool OrderBook::remove_level(
		const Instrument& instrument,
		OrderSide side,
		double price,
		long long timestamp_ns) {
	if (!is_valid_instrument(instrument)
			|| !std::isfinite(price)
			|| price <= 0.0
			|| timestamp_ns < 0
			|| (side != OrderSide::Buy && side != OrderSide::Sell)) {
		return false;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<Instrument, BookState, InstrumentLess>::iterator book_it =
		books_.find(instrument);
	if (book_it == books_.end() || is_stale(book_it->second, timestamp_ns)) {
		return false;
	}

	std::size_t removed = side == OrderSide::Buy
		? book_it->second.bids.erase(price)
		: book_it->second.asks.erase(price);
	if (removed == 0) {
		return false;
	}

	update_timestamp(book_it->second, timestamp_ns);
	return true;
}

bool OrderBook::update_last_trade(
		const Instrument& instrument,
		double price,
		long long timestamp_ns) {
	if (!is_valid_instrument(instrument)
			|| !std::isfinite(price)
			|| price <= 0.0
			|| timestamp_ns < 0) {
		return false;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	BookState& book = books_[instrument];
	if (is_stale(book, timestamp_ns)) {
		return false;
	}

	book.last_trade_price = price;
	update_timestamp(book, timestamp_ns);
	return true;
}

bool OrderBook::market_data(
		const Instrument& instrument,
		MarketData& out) const {
	std::lock_guard<std::mutex> lock(mutex_);
	std::map<Instrument, BookState, InstrumentLess>::const_iterator book_it =
		books_.find(instrument);
	if (book_it == books_.end()) {
		return false;
	}

	const BookState& book = book_it->second;
	if (book.bids.empty() && book.asks.empty() && book.last_trade_price == 0.0) {
		return false;
	}

	MarketData snapshot;
	snapshot.instrument = instrument;
	snapshot.bid = book.bids.empty() ? 0.0 : book.bids.begin()->first;
	snapshot.ask = book.asks.empty() ? 0.0 : book.asks.begin()->first;
	snapshot.last = book.last_trade_price;
	snapshot.timestamp_ns = book.timestamp_ns;
	if (!is_valid_market_data(snapshot)) {
		return false;
	}

	out = snapshot;
	return true;
}

std::vector<PriceLevel> OrderBook::copy_levels(
		const std::map<double, int, std::greater<double> >& levels) {
	std::vector<PriceLevel> result;
	result.reserve(levels.size());
	for (std::map<double, int, std::greater<double> >::const_iterator it =
			levels.begin(); it != levels.end(); ++it) {
		result.push_back(PriceLevel{it->first, it->second});
	}
	return result;
}

std::vector<PriceLevel> OrderBook::copy_levels(
		const std::map<double, int>& levels) {
	std::vector<PriceLevel> result;
	result.reserve(levels.size());
	for (std::map<double, int>::const_iterator it = levels.begin();
			it != levels.end(); ++it) {
		result.push_back(PriceLevel{it->first, it->second});
	}
	return result;
}

std::vector<PriceLevel> OrderBook::bid_levels(
		const Instrument& instrument) const {
	std::lock_guard<std::mutex> lock(mutex_);
	std::map<Instrument, BookState, InstrumentLess>::const_iterator book_it =
		books_.find(instrument);
	return book_it == books_.end()
		? std::vector<PriceLevel>()
		: copy_levels(book_it->second.bids);
}

std::vector<PriceLevel> OrderBook::ask_levels(
		const Instrument& instrument) const {
	std::lock_guard<std::mutex> lock(mutex_);
	std::map<Instrument, BookState, InstrumentLess>::const_iterator book_it =
		books_.find(instrument);
	return book_it == books_.end()
		? std::vector<PriceLevel>()
		: copy_levels(book_it->second.asks);
}

} // namespace trading
