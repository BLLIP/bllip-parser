// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License.  You may obtain
// a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.

// utility.h
//
// (c) Mark Johnson, 24th January 2005, last modified 14th November 2009
//
// modified 6th May 2002 to ensure write/read consistency, fixed 18th July 2002
// modified 14th July 2002 to include insert() (generic inserter)
// modified 26th September 2003 to use mapped_type instead of data_type
// 25th August 2004 added istream >> const char*
// 24th January 2005 added insert_newkey()
// 14th November 2009, added HERE, ASSERT()
//
// Defines:
//  loop macros foreach, cforeach
//  dfind (default find function)
//  afind (find function that asserts key exists)
//  insert_newkey (inserts a new key into a map)
//  insert (generic inserter into standard data structures)
//  disjoint (set operation)
//  first_lessthan and second_lessthan (compares elements of pairs)
//
// Simplified interfaces to STL routines:
//
//  includes (simplified interface)
//  set_intersection (simplified interface)
//  inserter (simplified interface)
//  max_element (simplified interface)
//  min_element (simplified interface)
//  hash functions for pairs, vectors, lists, slists and maps
//  input and output for pairs and vectors
//  resource_usage (interface improved)


#ifndef UTILITY_H
#define UTILITY_H

#include <algorithm>
// #include <boost/smart_ptr.hpp>    // Comment out this line if boost is not used
#include <cassert>
#include <cctype>
#include <cstdio>
#include <ext/hash_map>
#include <ext/hash_set>
#ifdef __clang__
#include <forward_list>
#else
#include <ext/slist>
#endif
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#if (__GNUC__ > 3) || (__GNUC__ >= 3 && __GNUC_MINOR__ >= 1)
#define EXT_NAMESPACE __gnu_cxx
#else
#define EXT_NAMESPACE std
#endif

namespace ext = EXT_NAMESPACE;

#ifdef __clang__
namespace EXT_NAMESPACE {
	template <class T> using slist = std::forward_list<T>;
}
#endif

// define some useful macros

#define HERE   __FILE__ << ":" << __LINE__ << ": In " << __func__ << "()"

// ASSERT() is a version of assert() that is always checked, no matter what
// NDEBUG is set to
//
#define ASSERT(expr) { if (!(expr)) { std::cerr << HERE << ", assertion \"" << __STRING(expr) << "\" failed." << std::endl; std::abort(); } }

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                              Looping constructs                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

// foreach is a simple loop construct
//
//   STORE should be an STL container
//   TYPE is the typename of STORE
//   VAR will be defined as a local variable of type TYPE::iterator
//
#define foreach(TYPE, VAR, STORE) \
   for (TYPE::iterator VAR = (STORE).begin(); VAR != (STORE).end(); ++VAR)

// cforeach is just like foreach, except that VAR is a const_iterator
//
//   STORE should be an STL container
//   TYPE is the typename of STORE
//   VAR will be defined as a local variable of type TYPE::const_iterator
//
#define cforeach(TYPE, VAR, STORE) \
   for (TYPE::const_iterator VAR = (STORE).begin(); VAR != (STORE).end(); ++VAR)


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                             Map searching                             //
//                                                                       //
// dfind(map, key) returns the key's value in map, or map's default      //
//   value if no such key exists (the default value is not inserted)     //
//                                                                       //
// afind(map, key) returns a reference to the key's value in map, and    //
//    asserts that this value exists                                     //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

// dfind(Map, Key) returns the value Map associates with Key, or the
//  Map's default value if no such Key exists
//
template <class Map, class Key>
inline typename Map::mapped_type dfind(Map& m, const Key& k)
{
  typename Map::iterator i = m.find(k);
  if (i == m.end())
    return typename Map::mapped_type();
  else
    return i->second;
}

template <class Map, class Key>
inline const typename Map::mapped_type dfind(const Map& m, const Key& k)
{
  typename Map::const_iterator i = m.find(k);
  if (i == m.end())
    return typename Map::mapped_type();
  else
    return i->second;
}


// afind(map, key) returns a reference to the value associated
//  with key in map.  It uses assert to check that the key's value
//  is defined.
//
template <class Map, class Key>
inline typename Map::mapped_type& afind(Map& m, const Key& k)
{
  typename Map::iterator i = m.find(k);
  assert(i != m.end());
  return i->second;
}

template <class Map, class Key>
inline const typename Map::mapped_type& afind(const Map& m, const Key& k)
{
  typename Map::const_iterator i = m.find(k);
  assert(i != m.end());
  return i->second;
}

//! insert_newkey(map, key, value) checks that map does not contain
//! key, and binds key to value.
//
template <class Map, class Key, class Value>
inline typename Map::value_type& 
insert_newkey(Map& m, const Key& k,const Value& v) 
{
  std::pair<typename Map::iterator, bool> itb 
    = m.insert(Map::value_type(k, v));
  assert(itb.second);
  return *(itb.first);
}  // insert_newkey()


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                        Insert operations                              //
//                                                                       //
///////////////////////////////////////////////////////////////////////////


template <typename T>
void insert(std::list<T>& xs, const T& x) {
  xs.push_back(x);
}

template <typename T>
void insert(std::set<T>& xs, const T& x) {
  xs.insert(x);
}

template <typename T>
void insert(std::vector<T>& xs, const T& x) {
  xs.push_back(x);
}


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                Additional versions of standard algorithms             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

template <typename Set1, typename Set2>
inline bool includes(const Set1& set1, const Set2& set2)
{
  return std::includes(set1.begin(), set1.end(), set2.begin(), set2.end());
}

template <typename Set1, typename Set2, typename Compare>
inline bool includes(const Set1& set1, const Set2& set2, Compare comp)
{
  return std::includes(set1.begin(), set1.end(), set2.begin(), set2.end(), comp);
}


template <typename InputIter1, typename InputIter2>
bool disjoint(InputIter1 first1, InputIter1 last1,
	      InputIter2 first2, InputIter2 last2)
{
  while (first1 != last1 && first2 != last2)
    if (*first1 < *first2)
      ++first1;
    else if (*first2 < *first1)
      ++first2;
    else // *first1 == *first2
      return false;
  return true;
}

template <typename InputIter1, typename InputIter2, typename Compare>
bool disjoint(InputIter1 first1, InputIter1 last1,
	      InputIter2 first2, InputIter2 last2, Compare comp)
{
  while (first1 != last1 && first2 != last2)
    if (comp(*first1, *first2))
      ++first1;
    else if (comp(*first2, *first1))
      ++first2;
    else // *first1 == *first2
      return false;
  return true;
}

template <typename Set1, typename Set2>
inline bool disjoint(const Set1& set1, const Set2& set2)
{
  return disjoint(set1.begin(), set1.end(), set2.begin(), set2.end());
}

template <typename Set1, typename Set2, typename Compare>
inline bool disjoint(const Set1& set1, const Set2& set2, Compare comp)
{
  return disjoint(set1.begin(), set1.end(), set2.begin(), set2.end(), comp);
}


template <typename Set1, typename Set2, typename OutputIterator>
inline OutputIterator set_intersection(const Set1& set1, const Set2& set2, 
				       OutputIterator result)
{
  return set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), result);
}

template <typename Set1, typename Set2, typename OutputIterator, typename Compare>
inline OutputIterator set_intersection(const Set1& set1, const Set2& set2, 
				       OutputIterator result, Compare comp)
{
  return set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), result, comp);
}


template <typename Container>
inline std::insert_iterator<Container> inserter(Container& container)
{
  return std::inserter(container, container.begin());
}

// max_element
//
template <class Es> inline typename Es::iterator max_element(Es& es)
{
  return std::max_element(es.begin(), es.end());
}

template <class Es> inline typename Es::const_iterator max_element(const Es& es)
{
  return std::max_element(es.begin(), es.end());
}

template <class Es, class BinaryPredicate> 
inline typename Es::iterator max_element(Es& es, BinaryPredicate comp)
{
  return std::max_element(es.begin(), es.end(), comp);
}

template <class Es, class BinaryPredicate> 
inline typename Es::const_iterator max_element(const Es& es, BinaryPredicate comp)
{
  return std::max_element(es.begin(), es.end(), comp);
}

// min_element
//
template <class Es> inline typename Es::iterator min_element(Es& es)
{
  return std::min_element(es.begin(), es.end());
}

template <class Es> inline typename Es::const_iterator min_element(const Es& es)
{
  return std::min_element(es.begin(), es.end());
}

template <class Es, class BinaryPredicate> 
inline typename Es::iterator min_element(Es& es, BinaryPredicate comp)
{
  return std::min_element(es.begin(), es.end(), comp);
}

template <class Es, class BinaryPredicate> 
inline typename Es::const_iterator min_element(const Es& es, BinaryPredicate comp)
{
  return std::min_element(es.begin(), es.end(), comp);
}

// first_lessthan and second_lessthan
//
struct first_lessthan {
  template <typename T1, typename T2>
  bool operator() (const T1& e1, const T2& e2) {
    return e1.first < e2.first;
  }
};

struct second_lessthan {
  template <typename T1, typename T2>
  bool operator() (const T1& e1, const T2& e2) {
    return e1.second < e2.second;
  }
};

// first_greaterthan and second_greaterthan
//
struct first_greaterthan {
  template <typename T1, typename T2>
  bool operator() (const T1& e1, const T2& e2) {
    return e1.first > e2.first;
  }
};

struct second_greaterthan {
  template <typename T1, typename T2>
  bool operator() (const T1& e1, const T2& e2) {
    return e1.second > e2.second;
  }
};


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                          hash<> specializations                       //
//                                                                       //
// These must be in namespace std.   They permit the corresponding STL   //
// container to be used as a key in an STL hash table.                   //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

namespace EXT_NAMESPACE {

  // hash function for bool
  //
  template <> struct hash<bool>
  {
    size_t operator() (bool b) const 
    {
      return b;
    } // operator()
  }; // hash<bool>{}

  // hash function for strings
  //
  template <> struct hash<std::string> 
  {
    size_t operator()(const std::string& s) const 
    {
      typedef std::string::const_iterator CI;
      
      unsigned long h = 0; 
      unsigned long g;
      CI p = s.begin();
      CI end = s.end();
      
      while (p!=end) {
	h = (h << 4) + (*p++);
	if ((g = h&0xf0000000)) {
	  h = h ^ (g >> 24);
	  h = h ^ g;
	}}
      return size_t(h);
    }  // operator()
  };  // hash<string>{}


  // hash function for arbitrary pairs
  //
  template<class T1, class T2> struct hash<std::pair<T1,T2> > {
    size_t operator()(const std::pair<T1,T2>& p) const
    {
      size_t h1 = hash<T1>()(p.first);
      size_t h2 = hash<T2>()(p.second);
      return h1 ^ (h1 >> 1) ^ h2 ^ (h2 << 1);
    }
  };


  // hash function for vectors
  //
  template<class T> struct hash<std::vector<T> > 
  { //  This is the fn hashpjw of Aho, Sethi and Ullman, p 436.
    size_t operator()(const std::vector<T>& s) const 
    {
      typedef typename std::vector<T>::const_iterator CI;

      unsigned long h = 0; 
      unsigned long g;
      CI p = s.begin();
      CI end = s.end();
      
      while (p!=end) {
	h = (h << 5) + hash<T>()(*p++);
	if ((g = h&0xff000000)) {
	  h = h ^ (g >> 23);
	  h = h ^ g;
	}}
      return size_t(h);
    }
  };

  // hash function for slists
  //
  template<class T> struct hash<ext::slist<T> > 
  { //  This is the fn hashpjw of Aho, Sethi and Ullman, p 436.
    size_t operator()(const ext::slist<T>& s) const 
    {
      typedef typename ext::slist<T>::const_iterator CI;

      unsigned long h = 0; 
      unsigned long g;
      CI p = s.begin();
      CI end = s.end();
      
      while (p!=end) {
	h = (h << 7) + hash<T>()(*p++);
	if ((g = h&0xff000000)) {
	  h = h ^ (g >> 23);
	  h = h ^ g;
	}}
      return size_t(h);
    }
  };

  // hash function for maps
  //
  template<typename T1, typename T2> struct hash<std::map<T1,T2> >
  {
    size_t operator()(const std::map<T1,T2>& m) const
    {
      typedef typename std::map<T1,T2> M;
      typedef typename M::const_iterator CI;
      
      unsigned long h = 0;
      unsigned long g;
      CI p = m.begin();
      CI end = m.end();

      while (p != end) {
	h = (h << 11) + hash<typename M::value_type>()(*p++);
	if ((g = h&0xff000000)) {
	  h = h ^ (g >> 23);
	  h = h ^ g;
	}}
      return size_t(h);
    }
  };
	
} // namespace EXT_NAMESPACE



///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                           Write/Read code                             //
//                                                                       //
// These routines should possess write/read invariance IF their elements //
// also have write-read invariance.  Whitespace, '(' and ')' are used as //
// delimiters.                                                           //
//                                                                       //
///////////////////////////////////////////////////////////////////////////


// Define istream >> const char* so that it consumes the characters from the
// istream.  Just as in scanf, a space consumes an arbitrary amount of whitespace.
//
inline std::istream& operator>> (std::istream& is, const char* cp)
{
  if (*cp == '\0')
    return is;
  else if (*cp == ' ') {
    char c;
    if (is.get(c)) {
      if (isspace(c))
	return is >> cp;
      else {
	is.unget();
	return is >> (cp+1);
      }
    }
    else {
      is.clear(is.rdstate() & ~std::ios::failbit);  // clear failbit
      return is;
    }
  }
  else {
    char c;
    if (is.get(c)) {
      if (c == *cp)
	return is >> (cp+1);
      else {
	is.unget();
	is.setstate(std::ios::failbit);
      }
    }
    return is;
  }
}


// Write out an auto_ptr object just as you would write out the pointer object
//
template <typename T> 
inline std::ostream& operator<< (std::ostream& os, const std::auto_ptr<T>& sp)
{
  return os << sp.get();
}

// Vectors
//
template <class T>
std::ostream& operator<< (std::ostream& os, const std::vector<T>& xs)
{
  os << '(';
  for (typename std::vector<T>::const_iterator xi = xs.begin(); xi != xs.end(); ++xi) {
    if (xi != xs.begin())
      os << ' ';
    os << *xi;
  }
  return os << ')';
}

template <class T>
std::istream& operator>> (std::istream& is, std::vector<T>& xs)
{
  char c;                          // This code avoids unnecessary copy
  if (is >> c) {                   // read the initial '('
    if (c == '(') {
      xs.clear();                  // clear the list
      do {
	xs.push_back(T());         // create a new elt in list
	is >> xs.back();           // read element
      }
      while (is.good());           // read as long as possible
      xs.pop_back();               // last read failed; pop last elt
      is.clear(is.rdstate() & ~std::ios::failbit);  // clear failbit
      if (is >> c && c == ')')     // read terminating ')'
	return is;                 // successful return
      else 
	is.setstate(std::ios::badbit); // something went wrong, set badbit
    }
    else                           // c is not '('
      is.putback(c);               //  put c back into input
  }
  is.setstate(std::ios::failbit);  // read failed, set failbit
  return is;
}

// Pairs
//
template <class T1, class T2>
std::ostream& operator<< (std::ostream& os, const std::pair<T1,T2>& p)
{
  return os << '(' << p.first << ' ' << p.second << ')';
}

template <class T1, class T2>
std::istream& operator>> (std::istream& is, std::pair<T1,T2>& p)
{
  char c;
  if (is >> c) {
    if (c == '(') {
      if (is >> p.first >> p.second >> c && c == ')')
	return is;
      else
	is.setstate(std::ios::badbit);
    }
    else
      is.putback(c);
  }
  is.setstate(std::ios::failbit);
  return is;
}

// Lists
//
template <class T>
std::ostream& operator<< (std::ostream& os, const std::list<T>& xs)
{
  os << '(';
  for (typename std::list<T>::const_iterator xi = xs.begin(); xi != xs.end(); ++xi) {
    if (xi != xs.begin())
      os << ' ';
    os << *xi;
  }
  return os << ')';
}

template <class T>
std::istream& operator>> (std::istream& is, std::list<T>& xs)
{
  char c;                          // This code avoids unnecessary copy
  if (is >> c) {                   // read the initial '('
    if (c == '(') {
      xs.clear();                  // clear the list
      do {
	xs.push_back(T());         // create a new elt in list
	is >> xs.back();           // read element
      }
      while (is.good());           // read as long as possible
      xs.pop_back();               // last read failed; pop last elt
      is.clear(is.rdstate() & ~std::ios::failbit);  // clear failbit
      if (is >> c && c == ')')     // read terminating ')'
	return is;                 // successful return
      else 
	is.setstate(std::ios::badbit); // something went wrong, set badbit
    }
    else                           // c is not '('
      is.putback(c);               //  put c back into input
  }
  is.setstate(std::ios::failbit);  // read failed, set failbit
  return is;
}

// Slists
//
template <class T>
std::ostream& operator<< (std::ostream& os, const ext::slist<T>& xs)
{
  os << '(';
  for (typename ext::slist<T>::const_iterator xi = xs.begin(); xi != xs.end(); ++xi) {
    if (xi != xs.begin())
      os << ' ';
    os << *xi;
  }
  return os << ')';
}

template <class T>
std::istream& operator>> (std::istream& is, ext::slist<T>& xs)
{
  char c; 
  if (is >> c) { 
    if (c == '(') {
      xs.clear();
      T e;
      if (is >> e) {   
	xs.push_front(e);
	typename ext::slist<T>::iterator xi = xs.begin();
	while (is >> e)
	  xi = xs.insert_after(xi, e);
	is.clear(is.rdstate() & ~std::ios::failbit);
	if (is >> c && c == ')')
	  return is;
	else 
	  is.setstate(std::ios::badbit); 
      }
      else { // empty list
	is.clear(is.rdstate() & ~std::ios::failbit);
	if (is >> c && c == ')')
	  return is;
	else  // didn't see closing ')'
	  is.setstate(std::ios::badbit);
      }
    }
    else  // didn't read '('
      is.putback(c);
  }
  is.setstate(std::ios::failbit);
  return is;
}

// Sets
//
template <class T>
std::ostream& operator<< (std::ostream& os, const std::set<T>& s)
{
  os << '(';
  for (typename std::set<T>::const_iterator i = s.begin(); i != s.end(); ++i) {
    if (i != s.begin())
      os << ' ';
    os << *i;
  }
  return os << ')';
}

template <class T>
std::istream& operator>> (std::istream& is, std::set<T>& s)
{
  char c;
  if (is >> c) {
    if (c == '(') {
      s.clear();
      T e;
      while (is >> e)
	s.insert(e);
      is.clear(is.rdstate() & ~std::ios::failbit);
      if (is >> c && c == ')')
	return is;
      else
	is.setstate(std::ios::badbit);
    }
    else
      is.putback(c);
  }
  is.setstate(std::ios::failbit);
  return is;
}

// Hash_sets
//
template <class T>
std::ostream& operator<< (std::ostream& os, const ext::hash_set<T>& s)
{
  os << '(';
  for (typename ext::hash_set<T>::const_iterator i = s.begin(); i != s.end(); ++i) {
    if (i != s.begin())
      os << ' ';
    os << *i;
  }
  return os << ')';
}

template <class T>
std::istream& operator>> (std::istream& is, ext::hash_set<T>& s)
{
  char c;
  if (is >> c) {
    if (c == '(') {
      s.clear();
      T e;
      while (is >> e)
	s.insert(e);
      is.clear(is.rdstate() & ~std::ios::failbit);
      if (is >> c && c == ')')
	return is;
      else
	is.setstate(std::ios::badbit);
    }
    else
      is.putback(c);
  }
  is.setstate(std::ios::failbit);
  return is;
}


// Maps
//
template <class Key, class Value>
std::ostream& operator<< (std::ostream& os, const std::map<Key,Value>& m)
{
  typedef std::map<Key,Value> M;
  os << '(';
  for (typename M::const_iterator it = m.begin(); it != m.end(); ++it) {
    if (it != m.begin())
      os << ' ';
    os << *it;
  }
  return os << ")";
}

template <class Key, class Value>
std::istream& operator>> (std::istream& is, std::map<Key,Value>& m)
{
  char c;
  if (is >> c) {
    if (c == '(') {
      m.clear();
      std::pair<Key,Value> e;
      while (is >> e)
	m.insert(e);
      is.clear(is.rdstate() & ~std::ios::failbit);
      if (is >> c && c == ')')
	return is;
      else
	is.setstate(std::ios::badbit);
    }
    else
      is.putback(c);
  }
  is.setstate(std::ios::failbit);
  return is;
}

// Hash_maps
//
template <class Key, class Value>
std::ostream& operator<< (std::ostream& os, const ext::hash_map<Key,Value>& m)
{
  typedef ext::hash_map<Key,Value> M;
  os << '(';
  for (typename M::const_iterator it = m.begin(); it != m.end(); ++it) {
    if (it != m.begin())
      os << ' ';
    os << *it;
  }
  return os << ")";
}

template <class Key, class Value>
std::istream& operator>> (std::istream& is, ext::hash_map<Key,Value>& m)
{
  char c;
  if (is >> c) {
    if (c == '(') {
      m.clear();
      std::pair<Key,Value> e;
      while (is >> e)
	m.insert(e);
      is.clear(is.rdstate() & ~std::ios::failbit);
      if (is >> c && c == ')')
	return is;
      else
	is.setstate(std::ios::badbit);
    }
    else
      is.putback(c);
  }
  is.setstate(std::ios::failbit);
  return is;
}


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                       Boost library additions                         //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

#ifdef BOOST_SHARED_PTR_HPP_INCLUDED

// enhancements to boost::shared_ptr so it can be used with hash
//
namespace std {
  template <typename T> struct equal_to<boost::shared_ptr<T> > 
    : public binary_function<boost::shared_ptr<T>, boost::shared_ptr<T>, bool> {
    bool operator() (const boost::shared_ptr<T>& p1, const boost::shared_ptr<T>& p2) const {
      return equal_to<T*>()(p1.get(), p2.get());
    }
  };
}  // namespace std

namespace EXT_NAMESPACE {
  template <typename T> struct hash<boost::shared_ptr<T> > {
    size_t operator() (const boost::shared_ptr<T>& a) const {
      return hash<T*>()(a.get());
    }
  };
}  // namespace ext

template <typename T> 
inline std::ostream& operator<< (std::ostream& os, const boost::shared_ptr<T>& sp)
{
  return os << sp.get();
}

#endif  // BOOST_SHARED_PTR_HPP_INCLUDED

struct resource_usage { };

inline std::ostream& operator<< (std::ostream& os, resource_usage r)
{
  FILE* fp = fopen("/proc/self/stat", "r");
  assert(fp);
  int utime;
  int stime;
  unsigned int vsize;
  unsigned int rss;
  int result = 
    fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %d %d %*d %*d %*d %*d"
           "%*u %*u %*d %u %u", &utime, &stime, &vsize, &rss);
  assert(result == 4);
  fclose(fp);
  // s << "utime = " << utime << ", stime = " << stime << ", vsize = " << vsize << ", rss = " << rss
;
  // return s << "utime = " << utime << ", vsize = " << vsize;
  return os << "utime " << float(utime)/1.0e2 << "s, vsize " 
	    << float(vsize)/1048576.0 << " Mb.";
}

#endif  // UTILITY_H
