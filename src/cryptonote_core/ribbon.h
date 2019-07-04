namespace service_nodes {

#define TRADE_OGRE_API "https://tradeogre.com/api/v1"
#define BITLIBER_API "https://bitliber.com/api/v1"

struct exchange_trade {
  uint64_t date;
  std::string type;
  std::string price;
  std::string quantity;
};

std::vector<exchange_trade> get_trades_from_ogre();
std::vector<exchange_trade> get_trades_from_bitliber();
std::vector<exchange_trade> trades_during_latest_20_blocks(std::vector<exchange_trade> trades);

}
