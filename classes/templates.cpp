#include "templates.hpp"

#include <cmath>

namespace trading {

bool is_valid_instrument(const Instrument& instrument) {
	return !instrument.symbol.empty()
		&& !instrument.exchange.empty()
		&& !instrument.currency.empty();
}

bool is_valid_market_data(const MarketData& market_data) {
	if (!is_valid_instrument(market_data.instrument)
			|| !std::isfinite(market_data.bid)
			|| !std::isfinite(market_data.ask)
			|| !std::isfinite(market_data.last)
			|| market_data.bid < 0.0
			|| market_data.ask < 0.0
			|| market_data.last < 0.0) {
		return false;
	}

	return market_data.bid == 0.0
		|| market_data.ask == 0.0
		|| market_data.ask >= market_data.bid;
}

bool is_valid_order_intent(const OrderIntent& intent) {
	if (!is_valid_instrument(intent.instrument) || intent.quantity <= 0) {
		return false;
	}

	if (intent.type == OrderType::Limit) {
		return std::isfinite(intent.limit_price) && intent.limit_price > 0.0;
	}

	return intent.type == OrderType::Market;
}

bool same_instrument(const Instrument& left, const Instrument& right) {
	return left.symbol == right.symbol
		&& left.exchange == right.exchange
		&& left.currency == right.currency;
}

} // namespace trading
