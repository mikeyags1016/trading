#include "strategy.hpp"

#include <stdexcept>

namespace trading {

Strategy::Strategy(const std::string& name)
	: name_(name) {
	if (name_.empty()) {
		throw std::invalid_argument("Strategy name must not be empty");
	}
}

const std::string& Strategy::name() const {
	return name_;
}

bool Strategy::on_market_data(
		const MarketData& market_data,
		OrderIntent& intent) {
	if (!is_valid_market_data(market_data)
			|| !generate_intent(market_data, intent)) {
		return false;
	}

	return is_valid_order_intent(intent)
		&& same_instrument(market_data.instrument, intent.instrument);
}

} // namespace trading
