namespace service_nodes {

#define TRADE_OGRE_API "https://tradeogre.com/api/v1"
#define BITLIBER_API "https://bitliber.com/api/v1"

struct exchange_trade {
  uint64_t date;
  std::string type;
  double price;
  double quantity;
};

//Trade API functions
bool get_trades_from_ogre(std::vector<exchange_trade> &trades);
bool get_trades_from_bitliber(std::vector<exchange_trade> &trades);
std::vector<exchange_trade> trades_during_latest_1_block(std::vector<exchange_trade> &trades);

//Price Functions
double price_over_x_blocks(int blocks);
double create_ribbon_red();
double create_ribbon_green(std::vector<exchange_trade> trades);
double create_ribbon_blue(std::vector<exchange_trade> trades);
double trades_weighted_mean(std::vector<exchange_trade> trades);

}
