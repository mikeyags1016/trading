#ifndef RISK_SYSTEM_HPP
#define RISK_SYSTEM_HPP

#include "../classes/templates.hpp"

#include <cstddef>
#include <string>

namespace trading {

struct RiskLimits {
	int max_order_quantity;
	int max_position_quantity;
	double max_order_notional;
	double max_daily_loss;
	std::size_t max_open_orders;
};

struct RiskSnapshot {
	int current_position;
	int pending_buy_quantity;
	int pending_sell_quantity;
	double realized_pnl_today;
	std::size_t open_order_count;
};

class RiskSystem {
public:
	explicit RiskSystem(const RiskLimits& limits);

	bool approve_order(
		const OrderIntent& intent,
		const MarketData& market_data,
		const RiskSnapshot& snapshot,
		std::string& rejection_reason) const;

	const RiskLimits& limits() const;

private:
	RiskLimits limits_;
};

} // namespace trading

#endif // RISK_SYSTEM_HPP
