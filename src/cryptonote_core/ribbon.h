namespace service_nodes {

#define TRADE_OGRE_API "https://tradeogre.com/api/v1"

struct trade_ogre_trade {
  uint64_t date;
  std::string type;
  std::string price;
  std::string quantity;
};

std::vector<trade_ogre_trade> get_trades_from_ogre();
std::vector<trade_ogre_trade> trades_during_latest_20_blocks(std::vector<trade_ogre_trade> trades);

}
