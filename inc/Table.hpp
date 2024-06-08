#ifndef TABLE_H
#define TABLE_H


#include <unordered_map>
#include <vector>

template <typename T> class Table {

  std::vector<T> table;
  std::unordered_map<std::string, uint32_t> map;

public:

  int put(std::string name, T elem) {
    int entry = table.size();
    table.push_back(elem);
    map[name] = entry;
    return entry;
  }

  T& get(uint32_t index) { return table[index]; };
  T& get(std::string name) { return table[map.at(name)]; };
  int32_t getIndex(std::string name) const { if ( exists(name) ) return map.at(name); else return -1; }
  bool exists(std::string name) const { return !(map.find(name) == map.end()); };
  uint32_t size() const { return table.size(); };
  void remove(std::string name) { if ( exists(name) ) { table.erase(table.begin() + map[name]); map.erase(name); }}
};



#endif