#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include <string>

namespace trading {

enum class OrderSide {
	Buy,
	Sell
};

enum class OrderType {
	Market,
	Limit
};

struct Instrument {
	std::string symbol;
	std::string exchange;
	std::string currency;
};

struct MarketData {
	Instrument instrument;
	double bid;
	double ask;
	double last;
	long long timestamp_ns;
};

struct OrderIntent {
	Instrument instrument;
	OrderSide side;
	OrderType type;
	int quantity;
	double limit_price;
};

bool is_valid_instrument(const Instrument& instrument);
bool is_valid_market_data(const MarketData& market_data);
bool is_valid_order_intent(const OrderIntent& intent);
bool same_instrument(const Instrument& left, const Instrument& right);

class IbkrAdapter {
public:
	virtual ~IbkrAdapter() {}

	virtual bool connect(const std::string& host, int port, int client_id) = 0;
	virtual void disconnect() = 0;
	virtual bool is_connected() const = 0;
	virtual bool subscribe_market_data(const Instrument& instrument) = 0;
	virtual bool submit_order(const OrderIntent& intent, int& broker_order_id) = 0;
	virtual bool cancel_order(int broker_order_id) = 0;
};

} // namespace trading

#endif // TEMPLATES_HPP
