#include "risk_system.hpp"

#include <cmath>
#include <stdexcept>

namespace trading {

RiskSystem::RiskSystem(const RiskLimits& limits)
	: limits_(limits) {
	if (limits_.max_order_quantity <= 0
			|| limits_.max_position_quantity <= 0
			|| !std::isfinite(limits_.max_order_notional)
			|| limits_.max_order_notional <= 0.0
			|| !std::isfinite(limits_.max_daily_loss)
			|| limits_.max_daily_loss <= 0.0
			|| limits_.max_open_orders == 0) {
		throw std::invalid_argument("Risk limits must all be positive");
	}
}

bool RiskSystem::approve_order(
		const OrderIntent& intent,
		const MarketData& market_data,
		const RiskSnapshot& snapshot,
		std::string& rejection_reason) const {
	rejection_reason.clear();

	if (!is_valid_order_intent(intent)) {
		rejection_reason = "Invalid order intent";
		return false;
	}
	if (!is_valid_market_data(market_data)
			|| !same_instrument(intent.instrument, market_data.instrument)) {
		rejection_reason = "Market data is invalid or for a different instrument";
		return false;
	}
	if (snapshot.pending_buy_quantity < 0
			|| snapshot.pending_sell_quantity < 0
			|| !std::isfinite(snapshot.realized_pnl_today)) {
		rejection_reason = "Risk snapshot contains invalid values";
		return false;
	}
	if (intent.quantity > limits_.max_order_quantity) {
		rejection_reason = "Order quantity exceeds the per-order limit";
		return false;
	}

	double reference_price = intent.limit_price;
	if (intent.type == OrderType::Market) {
		if (intent.side == OrderSide::Buy) {
			reference_price = market_data.ask > 0.0
				? market_data.ask
				: (market_data.last > 0.0 ? market_data.last : market_data.bid);
		} else {
			reference_price = market_data.bid > 0.0
				? market_data.bid
				: (market_data.last > 0.0 ? market_data.last : market_data.ask);
		}
	}
	if (!std::isfinite(reference_price) || reference_price <= 0.0) {
		rejection_reason = "No usable price is available for the order";
		return false;
	}
	if (reference_price * intent.quantity > limits_.max_order_notional) {
		rejection_reason = "Order notional exceeds the configured limit";
		return false;
	}

	const long long effective_position =
		static_cast<long long>(snapshot.current_position)
		+ snapshot.pending_buy_quantity
		- snapshot.pending_sell_quantity;
	const long long projected_position = effective_position
		+ (intent.side == OrderSide::Buy ? intent.quantity : -intent.quantity);
	const long long current_exposure = effective_position < 0
		? -effective_position
		: effective_position;
	const long long projected_exposure = projected_position < 0
		? -projected_position
		: projected_position;
	const bool reduces_exposure = projected_exposure < current_exposure;

	if (projected_exposure > limits_.max_position_quantity && !reduces_exposure) {
		rejection_reason = "Order would exceed the position limit";
		return false;
	}
	if (snapshot.open_order_count >= limits_.max_open_orders && !reduces_exposure) {
		rejection_reason = "Open-order limit has been reached";
		return false;
	}
	if (snapshot.realized_pnl_today <= -limits_.max_daily_loss
			&& !reduces_exposure) {
		rejection_reason = "Daily loss limit has been reached";
		return false;
	}

	return true;
}

const RiskLimits& RiskSystem::limits() const {
	return limits_;
}

} // namespace trading
