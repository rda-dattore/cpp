#ifndef MYMAP_HPP
#define   MYMAP_HPP

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <list>

namespace my {

template <class Entry>
class map {
public:
  Entry __dum_e;

  map() : __dum_e(),table(nullptr),table_capacity(103),num_entries(0),ssize(0),key_pad(),key_list(),last_entry(nullptr) { constructTable(); }
  map(size_t capacity) :  __dum_e(),table(nullptr),table_capacity(capacity),num_entries(0),ssize(0),key_pad(),key_list(),last_entry(nullptr) { constructTable(); }
  map(const map& source) : __dum_e(),table(nullptr),table_capacity(),num_entries(),ssize(),key_pad(),key_list(),last_entry(nullptr) { copy(source); }
  map(map&& source) : __dum_e(),table(nullptr),table_capacity(),num_entries(),ssize(),key_pad(),key_list(),last_entry(nullptr) { *this=std::move(source); }
  map& operator=(const map& source);
  map& operator=(map&& source);
  size_t capacity() const { return table_capacity; }
  void clear();
  bool empty() const { return (num_entries == 0); }
  bool found(const size_t& key,Entry& entry);
  bool found(const std::vector<size_t>& key,Entry& entry);
  bool found(const std::string& key,Entry& entry);
  void insert(const Entry& entry);
  void insert(Entry&& entry);
  const std::list<decltype(__dum_e.key)>& keys() { return key_list; }
  template <class Func> void keysort(Func f) { key_list.sort(f); }
  Entry& operator[](const std::string& key);
  template <class Key> void remove(const Key& key);
  void replace(const Entry& entry);
  size_t size() const { return num_entries; }

private:
  void copy(const map& source);
  void constructTable() {
    table.reset(new std::list<Entry>[table_capacity]);
    last_entry=nullptr;
  }
  bool foundEntry(const size_t& key,typename std::list<Entry>::iterator& it);
  bool foundEntry(const std::vector<size_t>& key,typename std::list<Entry>::iterator& it);
  bool foundEntry(const std::string& key,typename std::list<Entry>::iterator& it);
  size_t hash(const size_t& key);
  size_t hash(const std::vector<size_t>& key);
  size_t hash(std::string key);
  void keyWarn(size_t key) { std::cerr << "Warning: entry for key " << key << " exists - entry not added" << std::endl; }
  void keyWarn(const std::vector<size_t>& key);
  void keyWarn(std::string key) { std::cerr << "Warning: entry for key " << key << " exists - entry not added" << std::endl; }

  std::unique_ptr<std::list<Entry>[]> table;
  size_t table_capacity,num_entries,ssize;
  std::string key_pad;
  std::list<decltype(__dum_e.key)> key_list;
  Entry *last_entry;
};

template <class Entry>
map<Entry>& map<Entry>::operator=(const map& source)
{
  if (this == &source) {
    return *this;
  }
  copy(source);
  return *this;
}

template <class Entry>
map<Entry>& map<Entry>::operator=(map&& source)
{
  if (this == &source) {
    return *this;
  }
  table.reset(nullptr);
  table_capacity=source.table_capacity;
  table=std::move(source.table);
  key_list=std::move(source.key_list);
  source.table=nullptr;
  num_entries=source.num_entries;
  source.num_entries=0;
  ssize=source.ssize;
  last_entry=source.last_entry;
last_entry=nullptr;
  source.last_entry=nullptr;
  return *this;
}

template <class Entry>
void map<Entry>::copy(const map<Entry>& source)
{
  table.reset(nullptr);
  table_capacity=source.table_capacity;
  constructTable();
  last_entry=nullptr;
  for (size_t n=0; n < table_capacity; ++n) {
    table[n]=source.table[n];
    if (source.last_entry == &source.table[n].back()) {
	last_entry=&table[n].back();
    }
  }
  key_list=source.key_list;
  num_entries=source.num_entries;
  ssize=source.ssize;
}

template <class Entry>
void map<Entry>::clear()
{
  for (size_t n=0; n < table_capacity; ++n) {
    table[n].clear();
  }
  num_entries=0;
  key_list.clear();
  last_entry=nullptr;
}

template <class Entry>
bool map<Entry>::found(const size_t& key,Entry& entry)
{
  if (last_entry != nullptr && key == last_entry->key) {
    entry=*last_entry;
    return true;
  }
  else {
    typename std::list<Entry>::iterator it;
    if (foundEntry(key,it)) {
	entry=*it;
	return true;
    }
    else {
	return false;
    }
  }
}

template <class Entry>
bool map<Entry>::found(const std::vector<size_t>& keys,Entry& entry)
{
  if (last_entry != nullptr && keys == last_entry->key) {
    entry=*last_entry;
    return true;
  }
  else {
    typename std::list<Entry>::iterator it;
    if (foundEntry(keys,it)) {
	entry=*it;
	return true;
    }
    else {
	return false;
    }
  }
}

template <class Entry>
bool map<Entry>::found(const std::string& key,Entry& entry)
{
  if (last_entry != nullptr && key == last_entry->key) {
    entry=*last_entry;
    return true;
  }
  else {
    typename std::list<Entry>::iterator it;
    if (foundEntry(key,it)) {
	entry=*it;
	return true;
    }
    else {
	return false;
    }
  }
}

template <class Entry>
void map<Entry>::insert(const Entry& entry)
{
  typename std::list<Entry>::iterator it;

  if (foundEntry(entry.key,it)) {
    keyWarn(entry.key);
  }
  else {
    size_t hash_addr=hash(entry.key);
    table[hash_addr].emplace_back(entry);
    key_list.emplace_back(entry.key);
    ++num_entries;
    last_entry=&table[hash_addr].back();
  }
}

template <class Entry>
void map<Entry>::insert(Entry&& entry)
{
  typename std::list<Entry>::iterator it;

  if (foundEntry(entry.key,it)) {
    keyWarn(entry.key);
  }
  else {
    size_t hash_addr=hash(entry.key);
    table[hash_addr].emplace_back(std::move(entry));
    key_list.emplace_back(entry.key);
    ++num_entries;
    last_entry=&table[hash_addr].back();
  }
}

template <class Entry>
Entry& map<Entry>::operator[](const std::string& key)
{
  if (last_entry != nullptr && key == last_entry->key) {
    return *last_entry;
  }
  else {
    typename std::list<Entry>::iterator it;
    if (foundEntry(key,it)) {
	return *it;
    }
    else {
	Entry e;
	e.key=key;
	size_t hash_addr=hash(e.key);
	table[hash_addr].emplace_back(e);
	key_list.emplace_back(e.key);
	++num_entries;
	last_entry=&table[hash_addr].back();
	return *last_entry;
    }
  }
}

template <class Entry>
template <class Key> void map<Entry>::remove(const Key& key)
{
  typename std::list<Entry>::iterator it;

  if (foundEntry(key,it)) {
    table[hash(key)].erase(it);
    for (auto k_it=key_list.begin(),end=key_list.end(); k_it != end; ++k_it) {
	if (*k_it == key) {
	  key_list.erase(k_it);
	  break;
	}
    }
    --num_entries;
    last_entry=nullptr;
  }
}

template <class Entry>
void map<Entry>::replace(const Entry& entry)
{
  typename std::list<Entry>::iterator it;

  if (foundEntry(entry.key,it)) {
    *it=entry;
  }
  else {
    insert(entry);
    ++num_entries;
  }
}

template <class Entry>
bool map<Entry>::foundEntry(const size_t& key,typename std::list<Entry>::iterator& it)
{
  size_t hash_addr=hash(key);
  typename std::list<Entry>::iterator end=table[hash_addr].end();
  for (it=table[hash_addr].begin(); it != end; ++it) {
    if (key == it->key) {
	last_entry=&(*it);
	break;
    }
  }
  return (it != end);
}

template <class Entry>
bool map<Entry>::foundEntry(const std::vector<size_t>& key,typename std::list<Entry>::iterator& it)
{
  size_t hash_addr=hash(key);
  typename std::list<Entry>::iterator end=table[hash_addr].end();
  for (it=table[hash_addr].begin(); it != end; ++it) {
    bool is_key=true;
    for (size_t n=0; n < key.size(); ++n) {
	if (key[n] != it->key[n]) {
	  is_key=false;
	  break;
	}
    }
    if (is_key) {
	last_entry=&(*it);
	break;
    }
  }
  return (it != end);
}

template <class Entry>
bool map<Entry>::foundEntry(const std::string& key,typename std::list<Entry>::iterator& it)
{
  size_t hash_addr=hash(key);
  typename std::list<Entry>::iterator end=table[hash_addr].end();
  for (it=table[hash_addr].begin(); it != end; ++it) {
    if (key == it->key) {
	last_entry=&(*it);
	break;
    }
  }
  return (it != end);
}

template <class Entry>
size_t map<Entry>::hash(const size_t& key)
{
  return (key % table_capacity);
}

template <class Entry>
size_t map<Entry>::hash(const std::vector<size_t>& key)
{
  size_t n;
  size_t return_key=0;

  for (n=0; n < key.size(); ++n) {
    return_key+=key[n];
  }
  return (return_key % table_capacity);
}

template <class Entry>
size_t map<Entry>::hash(std::string key)
{
  std::vector<size_t> keylist;
  size_t n,m;

  if (ssize == 0) {
    ssize=sizeof(size_t);
  }
  if (key_pad.length() == 0) {
    for (n=0; n < ssize-1; ++n) {
	key_pad+=" ";
    }
  }
  if ( (key.length() % ssize) != 0) {
    key+=key_pad;
  }
  for (n=0,m=key.length()/ssize; n < m; ++n) {
    keylist.emplace_back((reinterpret_cast<size_t *>(const_cast<char *>(key.c_str())))[n]);
  }
  return hash(keylist);
}

template <class Entry>
void map<Entry>::keyWarn(const std::vector<size_t>& key)
{
  std::cerr << "Warning: entry for keys ";
  for (size_t n=0; n < key.size(); n++) {
    if (n > 0) {
	std::cerr << ", ";
    }
    std::cerr << key[n];
  }
  std::cerr << " exists - entry not added" << std::endl;
}

} // end namespace my

#endif
