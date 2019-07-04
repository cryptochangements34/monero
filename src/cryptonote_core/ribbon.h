namespace service_nodes {

struct trade_oger_trade {
  uint32_t date;
  std::string type;
  std::string price;
  std::string quantity;
};

std::vector<trade_oger_trade> get_trades_from_ogre();

}
