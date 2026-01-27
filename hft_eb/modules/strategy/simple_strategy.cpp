#include "../../include/framework.h"
#include <cstring>

class StrategyModule : public IModule {
public:
    void init(EventBus* bus, const ConfigMap& config) override {
        bus_ = bus;
        buy_thresh_ = std::stod(config.at("buy_thresh"));
        sell_thresh_ = std::stod(config.at("sell_thresh"));
        
        std::cout << "[Strategy] Range: [" << buy_thresh_ << ", " << sell_thresh_ << "]" << std::endl;

        bus_->subscribe(EVENT_MARKET_DATA, [this](void* d) {
            this->onTick(static_cast<MarketData*>(d));
        });
    }

    void onTick(MarketData* md) {
        if (md->last_price < buy_thresh_) {
            // 触发买单
            std::cout << "[Strategy] Price " << md->last_price << " < " << buy_thresh_ << " -> BUY!" << std::endl;
            sendOrder(md->symbol, 'B', md->last_price);
        } 
        else if (md->last_price > sell_thresh_) {
            // 触发卖单
            std::cout << "[Strategy] Price " << md->last_price << " > " << sell_thresh_ << " -> SELL!" << std::endl;
            sendOrder(md->symbol, 'S', md->last_price);
        }
    }

    void sendOrder(const char* symbol, char dir, double price) {
        OrderReq req;
        strncpy(req.symbol, symbol, 31);
        req.direction = dir;
        req.price = price;
        req.volume = 1;
        bus_->publish(EVENT_ORDER_REQ, &req);
    }

private:
    EventBus* bus_;
    double buy_thresh_;
    double sell_thresh_;
};

EXPORT_MODULE(StrategyModule)
