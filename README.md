# trading

- Market Making Trading Strat on Kalshi

Modules:
- Templates:
    -   Holding data structures for orders, market data, etc
- Network:
    - Register to a group and listen to a multicast network (address based, NASDAQ, CME) for market data via UDP
    - Kalshi uses WebSockets, so we won't need to implement multicast listening
- Market Data Handler: 
    - Takes in data from the exchange
        - SNAPSHOT:
            - GET https://external-api.kalshi.com/trade-api/v2/markets/{ticker}GET /markets/{ticker}: Market details
            - GET https://external-api.kalshi.com/trade-api/v2/markets/{ticker}/orderbook: Orderbook depth
        - Live Updates:
            - GET https://external-api.kalshi.com/trade-api/v2/live_data/milestone/{milestone_id}: Single ticker feed
- Order book builder:
    - Keep track of orders from the exchange
    - Keep track of our orders
    - Best bid/ask price/qty
        - YES bid price: highest_ask_dollars - highest_bid_dollars (1 - highest_no_dollars)
    - Order types: YES, NO position (between $0-$1.00)
    - YES and NO bids, use 99 price level ordered map to store number of orders
    - FIFO queue per level 
    - Hash map from order IDs to orders
    - Cached best-bid/best-ask pointers
- Order execution
- Order manager
- Strategy
    - For now market making, can experiment with other strats
- Risk system:
    - Checks based on rate limits
    - Prevent orders based on PnL, exposure, position limits
- Backtesting feature:
    - Kalshi has ways of providing market data for specific markets that we can use to verify/change our strategy