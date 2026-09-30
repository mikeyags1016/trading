#ifndef ORDER_BOOK_HPP
#define ORDER_BOOK_HPP

#include "../classes/templates.hpp"

#include <functional>
#include <map>
#include <mutex>
#include <vector>

namespace trading {

struct PriceLevel {
	double price;
	int quantity;
};

class OrderBook {
public:
	bool update_level(
		const Instrument& instrument,
		OrderSide side,
		double price,
		int quantity,
		long long timestamp_ns);

	bool remove_level(
		const Instrument& instrument,
		OrderSide side,
		double price,
		long long timestamp_ns);

	bool update_last_trade(
		const Instrument& instrument,
		double price,
		long long timestamp_ns);

	bool market_data(const Instrument& instrument, MarketData& out) const;
	std::vector<PriceLevel> bid_levels(const Instrument& instrument) const;
	std::vector<PriceLevel> ask_levels(const Instrument& instrument) const;

private:
	struct InstrumentLess {
		bool operator()(const Instrument& left, const Instrument& right) const;
	};

	struct BookState {
		std::map<double, int, std::greater<double> > bids;
		std::map<double, int> asks;
		double last_trade_price;
		long long timestamp_ns;

		BookState() : last_trade_price(0.0), timestamp_ns(0) {}
	};

	static bool is_stale(const BookState& book, long long timestamp_ns);
	static void update_timestamp(BookState& book, long long timestamp_ns);
	static std::vector<PriceLevel> copy_levels(
		const std::map<double, int, std::greater<double> >& levels);
	static std::vector<PriceLevel> copy_levels(const std::map<double, int>& levels);

	mutable std::mutex mutex_;
	std::map<Instrument, BookState, InstrumentLess> books_;
};

} // namespace trading

#endif // ORDER_BOOK_HPP
