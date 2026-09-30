#ifndef STRATEGY_HPP
#define STRATEGY_HPP

#include "../classes/templates.hpp"

#include <string>

namespace trading {

class Strategy {
public:
	explicit Strategy(const std::string& name);
	virtual ~Strategy() {}

	const std::string& name() const;

	// Returns true only when a valid order intent was produced.
	bool on_market_data(const MarketData& market_data, OrderIntent& intent);

protected:
	virtual bool generate_intent(
		const MarketData& market_data,
		OrderIntent& intent) = 0;

private:
	std::string name_;
};

} // namespace trading

#endif // STRATEGY_HPP
