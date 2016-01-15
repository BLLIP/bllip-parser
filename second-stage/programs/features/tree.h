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

// tree.h
//
// (c) Mark Johnson, 20th August 2001, 
// last modified (c) Mark Johnson, 28th February 2010 
// to comment out custom allocator (incompatible with OpenMP)
// 
// The tree structure can represent arbitrary trees.
// Each tree node contains:
// - the node's label
// - a pointer to its first child (in subtrees)
// - a pointer to the right sibling of this node
//
// IMPORTANT NOTE: there is no automatic memory management
// for tree nodes.  readtree_root returns a tree pointer.
// You should manually delete the tree pointer when you are
// done with it.  Use operator delete to do this (e.g. delete tp).
//
// The tree_node copy constructor and operator delete have "deep"
// copy semantics.  Pointers to tree_node have shallow semantics.
//
// map_filenamefile_trees always deletes each tree after it
// has been read, so in this case you don't have to worry about
// deleting the trees.
//
// read_filenamefile_trees loads a vector with all of the trees
// in the corpus.  If you decide to delete this vector you should
// manually delete the tree points also.  
//
// If you define a new type of tree label you may also want to override
// copy_treeptr (which does the actual tree copying).  This may be
// necessary if the label contains pointers to other nodes in the tree.
//
// tree_label{}
//  cat
//  tree_label()
//  root()
//  none()
//  punctuation()
//  is_root()
//  is_none()
//  is_punctuation()
//  is_conjunction()
//  is_closed_class()
//  is_functional()
//  simplified_cat()
//
// operator<<
// operator>>
//
// tree_node{}
//  tree_node()
//  ~tree_node()
//  operator=()
//  operator==()
//  operator<()
//  is_terminal()
//  is_preterminal()
//  is_nonterminal()
//  is_root()
//  is_none()
//  is_punctuation()
//  is_conjunction()
//  is_closed_class()
//  is_functional()
//  delete_this()
//  copy_tree()
//  exists_cut_p()
//  selective_copy()
//  preorder_find()
//  count()
//  size()
//  max_depth()
//  is_empty()
//  is_coordination()
//  is_adjunction()
//  is_last_nonpunctuation()
//  is_adjunction_site()
//  delete_unary_same_label_chains()
//  copy_without_empties()
//  copy_left_binarize()
//  copy_parent_annotate()
//  preorder()
//  postorder()
//  find()
//  preorder_ancestors()
//  terminals()
//  preterminals()
//  preterminal_nodes()
//
// copy_treeptr()
//
// equal_to<tree_node>{}
// less<tree_node>{}
// greater<tree_node>{}
// equal_to<tree_node*>{}
// less<tree_node*>{}
// hash<tree_node>{}
// hash<tree_node*>{}
//
// tree
//
// readtree_lineno
// readtree_filename
// readtree_root()
// readtree()
//
// read_filenamefile_trees()
// map_filenamefile_trees()
// map_regex_trees()
//
// operator<< for tree
// write_tree_noquote()
// write_tree_noquote_root()
// write_prolog_tree()
// display_tree()
//
// precrec_type{}
//  operator()  (accumulates precision and recall counts from 2 trees)
//  precision()
//  recall()
//  f_score()
//  error_rate()
// operator<< for precrec_type
// operator+= for precrec_type
// operator< for precrec_type

#ifndef TREE_H
#define TREE_H

#include "sym.h"
#include "symset.h"
#include "utility.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <vector>


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                tree_label                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

//! tree_label{} is one of the types of labels that trees can have
//
struct tree_label {
  typedef symbol cat_type;        //!< type of category
  typedef symset catset_type;     //!< type of sets of categories
  cat_type cat;                   //!< label category

  tree_label(const symbol& cat = symbol::undefined()) : cat(cat) { };

  // template <typename label_type>
  // tree_label(const label_type& label) : cat(label.cat) { };

  bool operator== (const tree_label& l) const {
    return cat == l.cat;
  }  // operator== ()

  bool operator< (const tree_label& l) const {
    return cat < l.cat;
  }  // operator< ()

  //! root() is the label that root nodes have
  //
  //  static cat_type root() { static cat_type root("ROOT"); return root; }
  static cat_type root() { static cat_type root("S1"); return root; }

  //! none() is the label of empty preterminal nodes
  //
  static cat_type none() { static cat_type none("-NONE-"); return none; }

  //! punctuation() is the set of punctuation categories
  //
  static const catset_type& punctuation() { 
    static const catset_type punctuation("'' : # , . `` -LRB- -RRB-");
    return punctuation;
  }  // tree_label::punctuation()

  //! conjunction() is the set of conjunction categories
  //
  static const catset_type& conjunction() { 
    static const catset_type conjunction("CC CONJP");
    return conjunction;
  }  // tree_label::conjunction()

  //! closed_class() is the set of POS labels of closed-class
  //! lexical items
  //
  static const catset_type& closed_class() {
    static const catset_type closed_class(
	   "CC DT EX IN MD PDT POS PRP PRP$ PRT RP TO UH WDT WP WP$ WRB $");
    return closed_class;
  }  // tree_label::closed_class()

  //! functional_pos() is the set of POS labels of function word
  //! lexical items
  //
  static const catset_type& functional() {
    static const catset_type functional(
	   "CC DT EX IN MD POS PRP PRP$ PRT RP TO WDT WP WP$ WRB");
    return functional;
  }  // tree_label::functional()

  //! is_root() is true of root labels
  //
  bool is_root() const { return cat == root(); }

  //! is_none() is true of empty POS node labels
  //
  bool is_none() const { return cat == none(); }

  //! is_punctuation() is true of punctuation POS labels
  //
  bool is_punctuation() const { return punctuation().contains(cat); }

  //! is_conjunction() is true of conjunction POS labels
  //
  bool is_conjunction() const { return conjunction().contains(cat); }

  //! is_closed_class() is true of closed class POS labels
  //
  bool is_closed_class() const { return closed_class().contains(cat); }

  //! is_functional() is true of closed class POS labels
  //
  bool is_functional() const { return functional().contains(cat); }

  //! simplified_cat() returns the category up to the first word-internal hypen.
  //
  cat_type simplified_cat() const {
    const std::string& s = cat.string_reference();
    std::string::size_type pos = s.find_first_of("-=|^", 1);
    if (pos != std::string::npos && pos+1 < s.size())
      return cat_type(s.substr(0, pos));
    else
      return cat;
  }  // tree_label::simplified_cat()

};  // tree_label{}

namespace EXT_NAMESPACE {
  template <> struct hash<tree_label> {
    size_t operator()(const tree_label& l) const {
      return hash<symbol>()(l.cat);
    }
  };  // hash<tree_label>{}
}; // namespace EXT_NAMESPACE

//! operator>>() reads a label, stopping at eof or whitespace or a parenthesis
//
inline std::istream& operator>> (std::istream& is, tree_label& label)
{
  std::string s;
  char c;
  while (is.get(c))
    if (isspace(c) || c == ')' || c == '(') {
      is.unget();
      break;
    }
    else
      s.push_back(c);
  label.cat = tree_label::cat_type(s);
  return is;
}  // operator>>()

inline std::ostream& operator<< (std::ostream& os, const tree_label& label)
{
  return os << label.cat.c_str();
}  // operator<<()


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                            tree_node{}                                //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

// forward declarations
//
template <typename label_type> class tree_node;

template <typename label_type> 
tree_node<label_type>* copy_treeptr(const tree_node<label_type>* tp);

//! tree_node{} is a template parameterized by the node label type.
//! tree_nodes have "deep" copy, deletion and equality semantics (i.e.,
//! the children of the node are always copied/deleted/compared when
//! the parent is).  (Pointers to tree_nodes have "shallow" semantics).
//
template <typename label_type0=tree_label> 
class tree_node {
public:
  typedef label_type0 label_type;

  label_type label;

  tree_node* child;   // subtrees
  tree_node* next;    // sibling

  tree_node() : label(label_type()), child(NULL), next(NULL) { }

  tree_node(const label_type& label, tree_node* child = NULL,
	    tree_node* next = NULL) 
    : label(label), child(child), next(next) { }

  tree_node(const typename tree_label::cat_type& cat,
	    tree_node* child = NULL, tree_node* next = NULL) 
    : label(tree_label(cat)), child(child), next(next) { }

  //! The tree_node() copy constructor makes "deep" copies.
  //
  template <typename label_type1>
  tree_node(const tree_node<label_type1>& node) 
    : label(node.label),
      child(node.child ? new tree_node<label_type0>(*node.child) : NULL),
      next(node.next ? new tree_node<label_type0>(*node.next) : NULL)
  { } // tree_node::tree_node()
	    
  ~tree_node() { delete child; delete next; }

  //! Equals makes "deep" copies of trees.
  //
  tree_node& operator= (const tree_node& t) {
    if (this != &t) {
      delete child;
      delete next;
      this->label = t.label;
      if (t.child != NULL)
	this->child = t.child->copy_tree();
      else
	this->child = NULL;
      if (t.next == NULL)
	this->next = NULL;
      else
	this->next = t.next->copy_tree();
    }
    return *this;
  }  // tree_node::operator=()

  //! Equality looks at the entire tree, not just this node.
  //
  bool operator== (const tree_node& t) const {
    if (this == &t) return true;
    if (label != t.label) return false;
    return (child == t->child 
	    || (child != NULL && t->child != NULL && *child == *t->child))
      && (next == t->next
	  || (next != NULL && t->next != NULL && *next == *t->next));
  }  // tree_node::operator==()

  //! < looks at the entire tree, not just this node.
  //
  bool operator< (const tree_node& t) const {
    if (this == &t) return false;
    if (label < t.label) return true;
    if (t.label < label) return false;
    // label == t.label
    if (child == t->child) return false;
    if (child == NULL) return true;
    if (t->child == NULL) return false;
    if (*child < *t->child) return true;
    if (*t->child < *child) return false;
    // *child == *t->child
    if (next == t->next) return false;
    if (next == NULL) return true;
    if (t->next == NULL) return false;
    return (*next < *t->next);
  }  // tree_node::operator<()

private:
  /*
  struct cache {        // for new and delete
    tree_node* freelist;
    tree_node* freeblock;
    unsigned int freeblockindex;

    cache() : freelist(NULL), freeblock(NULL), freeblockindex(0) {}

    inline tree_node* alloc() {
      if (freelist) {
	tree_node* f = freelist;
	freelist = f->next;
	return f;
      }
      if (freeblockindex) 
	return freeblock + (--freeblockindex);
      freeblockindex = (16777216-64)/sizeof(tree_node);  // grab a 16Mb chunk at a time
      freeblock = (tree_node *) malloc(sizeof(tree_node)*freeblockindex);
      assert(freeblock);
      return freeblock + (--freeblockindex);
    }  // tree_node::cache::alloc()

    inline void free(tree_node* tp) {
      tp->next = freelist;
      freelist = tp;
    }
  };  // tree_node::cache{}
    
  inline static cache& getcache() {
    static cache c;
    return c;
  }  // tree_node::getcache()
  */
public:
  /*
  inline void* operator new (size_t size) {
    assert(size == sizeof(tree_node));
    return (void *) getcache().alloc();
  }  // tree_node::operator new()

  inline void operator delete(void *p, size_t size) {
    assert(size == sizeof(tree_node));
    getcache().free((tree_node *) p);
  }  // tree_node::operator delete()
  */
  //! is_terminal() is true of terminal nodes
  //
  bool is_terminal() const {
    return child == NULL;
  }  // tree_node::is_terminal()

  //! is_preterminal() is true of preterminal nodes
  //
  bool is_preterminal() const {
    return child && child->is_terminal();
  }  // tree_node::is_preterminal()

  //! is_nonterminal() is true of nonterminal nodes
  //
  bool is_nonterminal() const {
    return child && child->child;
  }  // tree_node::is_nonterminal() 

  //! is_root() is true of nodes with the ROOT label
  //
  bool is_root() const {
    return is_nonterminal() && label.is_root();
  }  // tree_node::is_root()

  //! is_none() is true of preterminal nodes with the label -NONE-
  //
  bool is_none() const {
    return is_preterminal() && label.is_none();
  }  // tree_node::is_none()

  //! is_punctuation() is true of punctuation preterminal nodes
  //
  bool is_punctuation() const {
    return is_preterminal() && label.is_punctuation();
  }  // tree_node::is_punctuation()

  //! is_conjunction() is true if this node is a preterminal whose
  //! label is a conjunction category (CC).
  //
  bool is_conjunction() const {
    return is_preterminal() && label.is_conjunction();
  }  // tree_node::is_conjunction()

  //! is_closed_class() is true of preterminal nodes of closed
  //! class categories.
  //
  bool is_closed_class() const {
    return is_preterminal() && label.is_closed_class();
  }  // tree_node::is_closed_class()

  //! is_functional() is true of preterminal nodes of functional
  //! categories.
  //
  bool is_functional() const {
    return is_preterminal() && label.is_functional();
  }  // tree_node::is_functional()

  //! delete_this() disconnects a node from its children and the
  //! deletes (i.e., frees) the node.
  //
  void delete_this() { child = next = NULL; delete this; }

  //! copy_tree() returns a pointer to a ``deep copy'' of this node
  //! and all of the nodes it points to.  Modified so that it defaults
  //! to the copy_tree() function (which can be overridden).
  //
  tree_node* copy_tree() const {
    return copy_treeptr(this);
  }  // tree_node::copy_tree()

  //! exists_cut_p() is true if there is a cut through this tree
  //! such that every node satsifies P()
  //
  template <typename P> bool exists_cut_p(P& p) const {
    if (p(this))
      return true;
    if (!child)
      return false;
    for (const tree_node* c = child; c; c = c->next)
      if (!c->exists_cut_p(p))
	return false;
    return true;
  }  // exists_cut_p()

  //! selective_copy() returns a copy of this node with all subtrees
  //! that satisfy ignore_subtree removed and all nodes that satisfy
  //! ignore_node removed.
  //
  template <typename IgnoreSubtree, typename IgnoreNode>
  tree_node* selective_copy(IgnoreSubtree& ignore_subtree, 
			    IgnoreNode& ignore_node,
			    tree_node* endp = NULL) const 
  {
    if (exists_cut_p(ignore_subtree)) 
      return next ? next->selective_copy(ignore_subtree, ignore_node, endp) 
	          : endp;
    else if (ignore_node(this)) {
      assert(child); // shouldn't delete a terminal, otherwise tree is ill-formed
      return child->selective_copy(ignore_subtree, 
				   ignore_node,
				   next ? next->selective_copy(ignore_subtree, 
							       ignore_node, 
							       endp) 
				        : endp);
    }
    else {
      tree_node* c = 
	new tree_node(label, 
	     child ? child->selective_copy(ignore_subtree, ignore_node) : NULL, 
	     next ? next->selective_copy(ignore_subtree, ignore_node, endp) : endp);
      return c;
    }
  }  // selective_copy()

  //! preorder_find() returns the first node for which pred is true
  //
  template <typename Pred> const tree_node* preorder_find(Pred pred) const
  {
    if (pred(this))
      return this;
    if (child) 
      if (const tree_node* rc = child->preorder_find(pred))
	return rc;
    if (next)
      if (const tree_node* rn = next->preorder_find(pred))
	return rn;
    return NULL;
  }  // tree_node::preorder_find()

  //! count() counts the number of nodes on which pred is true
  //
  template <typename Pred> unsigned int count(Pred pred, unsigned int cnt = 0) const
  {
    if (pred(this))
      ++cnt;
    if (child)
      cnt = child->count(pred, cnt);
    return next ? next->count(pred, cnt) : cnt;
  }  // tree_node::count()

private:
  //! helper function for size()
  //
  unsigned int size_helper(unsigned int n) const
  {
    ++n;
    if (child)
      n = child->size_helper(n);
    return next ? next->size_(n) : n;
  }

public:
  //! size() returns the number of nodes in tree
  //
  unsigned int size() const
  {
    return size_helper(0);
  }  // tree_node::size()

  //! max_depth() returns the length of the longest path of nodes to a terminal.
  //! All siblings are at same depth, children are at depth 1, etc.
  //
  unsigned int max_depth() const
  {
    return std::max(child ? 1+child->max_depth() : 0,
		    next ? next->max_depth() : 0);
  }  // tree_node::max_depth()

  //! is_empty() is true iff all terminals have POS -NONE-
  //
  bool is_empty() const
  {
    if (label.is_none())
      return true;
    if (!child)
      return false;
    for (const tree_node* c = child; c; c = c->next)
      if (!c->is_empty())
	return false;
    return true;
  }  // tree_node::is_empty()

  //! is_coordination() is true iff one of the non-first, non-last children 
  //!  is a conjunction (the first and last child are ignored so constructions 
  //!  beginning with a conjunction don't count as coordinated).
  //
  bool is_coordination() const 
  {
    if (!is_nonterminal())
      return false;
    const tree_node* c = child;
    if (c != NULL && c->next != NULL)
      for (c = c->next; c->next != NULL; c = c->next)
	if (c->next != NULL && c->is_conjunction())
	  return true;
    return false;
  }  // tree_node::is_coordination()

  //! is_adjunction() is true iff all of the non-punctuation children
  //! have the same label as the parent.
  //
  bool is_adjunction() const 
  {
    if (!is_nonterminal())
      return false;
    for (const tree_node* c = child; c != NULL; c = c->next) 
      if (c->label.cat != label.cat && !c->is_punctuation())
	return false;
    return true;
  }  // tree_node::is_adjunction()

  //! is_last_nonpunctuation() is true iff all right siblings are 
  //!  punctuation.
  //
  bool is_last_nonpunctuation() const
  {
    const tree_node* sibling = next;
    while (sibling && sibling->is_punctuation())
      sibling = sibling->next;
    return sibling == NULL;
  }  // tree_node::is_last_nonpunctuation()

  //! is_adjunction_site() is true iff all but one child is empty(), and
  //! that child has the same label as this node.
  //
  bool is_adjunction_site() const
  {
    if (!is_nonterminal())
      return false;

    assert(child != NULL);

    int non_empty_same(0), empty(0);

    for (const tree_node* c = child; c; c = c->next) {
      if (c->is_empty()) 
	++empty;
      else {
	if (c->label.cat != label.cat)
	  return false;
	else {
	  if (non_empty_same == 0) 
	    ++non_empty_same;
	  else
	    return false;
	}}}

    // if (empty == 0) {
    //   assert(!(child->child || child->next)); // Unary branching node with same category
    //   return false;
    // }
    
    return non_empty_same == 1;
  }   // tree_node::is_adjunction_site()

  //! delete_unary_same_label_chains() excises and deletes nodes with 
  //! repeated labels in unary chains.
  //
  void delete_unary_same_label_chains() { // ignores terminals
    if (child) {
      while (child->child && !child->next && child->label.cat == label.cat) {
 	tree_node* grandchild = child->child;
 	child->delete_this();
 	child = grandchild;
      }
      child->delete_unary_same_label_chains();
    }
    if (next)
      next->delete_unary_same_label_chains();
  }  // tree_node::delete_unary_same_label_chains()


  //! copy_without_empties() returns a ptr to a copy of this subtree with all
  //! empty nodes deleted.
  //
  tree_node* copy_without_empties(bool delete_adjunctions = false, 
				  tree_node* endp = NULL) const {
    if (is_empty()) 
      return next ? next->copy_without_empties(delete_adjunctions, endp) : endp;
    else if (delete_adjunctions && is_adjunction_site()) {
      assert(child);
      return child->copy_without_empties(delete_adjunctions,
					 next ? next->copy_without_empties(delete_adjunctions,
									   endp) : endp);
    }
    else {
      tree_node* c = new tree_node(label, 
				   child ? child->copy_without_empties(delete_adjunctions) : NULL, 
				   next ? next->copy_without_empties(delete_adjunctions, endp) : endp);
      return c;
    }
  }  // tree_node::copy_without_empties()


  //! copy_left_binarize() returns a pointer to a left binarized copy of
  //! this tree.
  //
  tree_node* copy_left_binarize() const {
    tree_node* next_copy = NULL;
    if (next) {
      next_copy = next->copy_left_binarize();
      if (next_copy->next)
	next_copy = new tree_node(label_type(), next_copy, NULL);
    }
    return new tree_node(label,
			 child ? child->copy_left_binarize() : NULL,
			 next_copy);
  }  // tree_node::copy_left_binarize()

  //! copy_parent_annotate() returns a pointer to a parent-annotated 
  //! copy of this tree.
  //
  tree_node* copy_parent_annotate(symbol parent_cat=symbol::undefined()) const {
    tree_node* t = new tree_node(label);
    assert(child);
    if (is_preterminal() || is_empty()) 
      t->child = child->copy_tree();
    else {
      t->child = child->copy_parent_annotate(label.cat);
      if (parent_cat.is_defined())
	t->label.cat = symbol(label.cat.string_reference() + "_" 
			      + parent_cat.string_reference());
    }
    if (next)
      t->next = next->copy_parent_annotate(parent_cat);
    return t;
  }  // tree_node::copy_parent_annotate()

  //! preorder() calls the function object fn() on every subtree
  //! in a preorder traversal.
  //
  template <typename Fn> void preorder(Fn& fn) {
    fn(this);
    if (child) 
      child->preorder(fn);
    if (next)
      next->preorder(fn);
  }
    // tree_node::preorder()// tree_node::preorder()

  template <typename Fn> void preorder(Fn& fn) const {
    fn(this);
    if (child) 
      child->preorder(fn);
    if (next)
      next->preorder(fn);
  }  // tree_node::preorder()

  //! postorder() calls the function object fn() on every subtree
  //! in a postorder traversal.
  //
  template <typename Fn> void postorder(Fn& fn) {
    if (child) 
      child->postorder(fn);
    fn(this);
    if (next)
      next->postorder(fn);
  }  // tree_node::postorder()

  template <typename Fn> void postorder(Fn& fn) const {
    if (child) 
      child->postorder(fn);
    fn(this);
    if (next)
      next->postorder(fn);
  }  // tree_node::postorder()

  //! find() returns the first node in a preorder traversal
  //! for which the function object fn() is true.
  //
  template <typename Fn> bool find(Fn fn) const {
    return fn(this) || (child && child->find(fn)) || (next && next->find(fn));
  }  // tree_node::find()


private:
  template <typename Fn> 
  void preorder_ancestors(Fn& fn, std::vector<const tree_node*>& ancestors)  
  {
    ancestors.push_back(this);
    fn(ancestors);
    if (child) {
      child->preorder_ancestors(fn, ancestors);
    }
    ancestors.pop_back();
    if (next)
      next->preorder_ancestors(fn, ancestors);
  }

public:

  //! preorder_ancestors() calls fn(as) where as.back() is the 
  //! node being visited, as[0] is the root node and as[i-1] is
  //! the parent of as[i].
  //
  template <typename Fn> void preorder_ancestors(Fn& fn) 
  {
    std::vector<const tree_node*> ancestors;
    preorder_ancestors(fn, ancestors);
    assert(ancestors.empty());
  }  // tree_node::preorder_ancestors()

  //! terminals() appends the terminals to terms, optionally skipping empty nodes
  //
  template <typename Ss> 
  void terminals(Ss& terms, bool include_empty=false) const
  {
    if (is_terminal()) 
      terms.push_back(label.cat);
    else { // child != NULL
      if ((!label.is_none())||include_empty)
	child->terminals(terms, include_empty);
    }
    if (next != NULL)
      next->terminals(terms, include_empty);
  }  // tree_node::terminals()

  //! preterminals() appends the preterminal categories to preterms, 
  //! optionally skipping empty nodes
  //
  template <typename Ss> 
  void preterminals(Ss& preterms, bool include_empty=false) const
  {
    assert(child != NULL);
    if (is_preterminal()) {
      if ((!label.is_none())||include_empty)
	preterms.push_back(label.cat);
    }
    else   // non-terminal
      child->preterminals(preterms, include_empty);
    if (next != NULL)
      next->preterminals(preterms, include_empty);
  }  // tree_node::preterminals()

  //! preterminal_nodes() appends the preterminal nodes to preterms, 
  //! optionally skipping empty nodes
  //
  template <typename TPtrs> 
  void preterminal_nodes(TPtrs& preterms, bool include_empty=false) const
  {
    assert(child != NULL);
    if (is_preterminal()) {
      if ((!label.is_none())||include_empty)
	preterms.push_back(this);
    }
    else   // non-terminal
      child->preterminal_nodes(preterms, include_empty);
    if (next != NULL)
      next->preterminal_nodes(preterms, include_empty);
  }  // tree_node::preterminal_nodes()

};  // tree_node{}


//! copy_treeptr() copies a tree node pointer.  It is a function so it
//!  can be overriden for specialized tree label types.
//
template <typename label_type>
tree_node<label_type>* copy_treeptr(const tree_node<label_type>* tp) {
  return tp == NULL ? NULL : 
    new tree_node<label_type>(tp->label, copy_treeptr(tp->child), copy_treeptr(tp->next));
}  // copy_tree()


namespace std {

  template <typename label_type> struct equal_to<tree_node<label_type> >
    : public binary_function<tree_node<label_type>, tree_node<label_type>, bool> {
    bool operator() (const tree_node<label_type>& t1, const tree_node<label_type>& t2) const {
      return equal_to<label_type>()(t1.label, t2.label) 
	     && equal_to<tree_node<label_type>* >()(t1.next, t2.next)
	     && equal_to<tree_node<label_type>* >()(t1.child, t2.child);
    }
  };  // equal_to<tree_node<label_type> >{}

  template <typename label_type> struct less<tree_node<label_type> >
    : public binary_function<tree_node<label_type>, tree_node<label_type>, bool> {
    bool operator() (const tree_node<label_type>& t1, const tree_node<label_type>& t2) const {
      if (less<label_type>()(t1.label, t2.label)) return true;
      if (less<label_type>()(t2.label, t1.label)) return false;
      if (less<tree_node<label_type>*>()(t1.child, t2.child)) return true;
      if (less<tree_node<label_type>*>()(t2.child, t1.child)) return false;
      return less<tree_node<label_type>*>()(t1.next, t2.next);
    }
  };  // less<tree_node<label_type> >{}
  
  template <typename label_type> struct greater<tree_node<label_type> >
    : public binary_function<tree_node<label_type>, tree_node<label_type>, bool> {
    bool operator() (const tree_node<label_type>& t1, const tree_node<label_type>& t2) const {
      if (greater<label_type>()(t1.label, t2.label)) return true;
      if (greater<label_type>()(t2.label, t1.label)) return false;
      if (greater<tree_node<label_type>*>()(t1.child, t2.child)) return true;
      if (greater<tree_node<label_type>*>()(t2.child, t1.child)) return false;
      return greater<tree_node<label_type>*>()(t1.next, t2.next);
    }
  };  // greater<tree_node<label_type> >{}

  //! equal_to<tree_node<label_type>*>{} defines a "deep" equality even for tree pointers.
  //
  template <typename label_type> struct equal_to<tree_node<label_type>*> 
    : public binary_function<tree_node<label_type>*,tree_node<label_type>*,bool> {
    bool operator() (const tree_node<label_type>* t1, const tree_node<label_type>* t2) const {
      if (!t1 && !t2) return true;
      if (!t1 || !t2) return false;
      if (t1 == t2) return true;
      return equal_to<tree_node<label_type> >()(*t1, *t2);
    }
  };  // equal_to<tree_node<label_type>*>{}

  //! less<tree_node<label_type>*> defines a "deep" ordering even for tree pointers.
  //
  template <typename label_type> struct less<tree_node<label_type>*>
    : public binary_function<tree_node<label_type>*,tree_node<label_type>*,bool> {
    bool operator() (const tree_node<label_type>* t1, const tree_node<label_type>* t2) const {
      if (!t1 && !t2) return false;
      if (!t1 && t2) return true;
      if (!t2 && t1) return false;
      if (t1 == t2) return false;
      return less<tree_node<label_type> >()(*t1, *t2);
    }
  };  // less<tree_node<label_type>*>{}

}  // namespace std

namespace EXT_NAMESPACE {
  template <typename label_type> struct hash<tree_node<label_type> > {
    typedef tree_node<label_type> tree;

    size_t operator()(const tree& t) const {
      size_t g, h = 2*hash<label_type>()(t.label) + 3*hash<tree*>()(t.next) + 5*hash<tree*>()(t.child);
      if ((g = h&0xf0000000)) {
	h = h ^ (g >> 24);
	h = h ^ g;
      }
      return h;
    }  // hash<tree_node<label_type> >::operator()

  };

  template <typename label_type> struct hash<tree_node<label_type>*> {
    typedef tree_node<label_type> tree;
    size_t operator()(const tree* t) const {
      if (t) 
	return hash<tree>()(*t);
      else
	return 0;
    }
  };
}; // namespace EXT_NAMESPACE


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                        tree reading routines                          //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

template <typename label_type>
std::istream& operator>> (std::istream& is, tree_node<label_type>*& tp) {
  typedef tree_node<label_type> tree;
  char c;
  if (is >> c) {
    if (c == ')')
      tp = NULL;
    else if (c == '(') {
      label_type label;
      is >> label;
      tp = new tree(label);
      if (is >> tp->child)
	for (tree* child = tp->child; is && child != NULL; child = child->next)
	  is >> child->next;
    }
    else {
      is.unget();
      label_type label;
      is >> label;
      tp = new tree(label);
    }
  }
  else
    tp = NULL;
  return is;
}

typedef tree_node<> tree;


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                       routines from read-tree.l                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

extern int readtree_lineno;              // defined in read-tree.l
extern const char* readtree_filename;

//! hallucinate a ROOT node label, needed for Penn TB trees
tree* readtree_root(FILE* fp, bool downcase_flag = false);  
//! don't hallucinate a ROOT node label, needed for BLLIP trees
tree* readtree(FILE* fp, bool downcase_flag = false);

//! read a tree from a C string
tree* readtree_root(const char* str, bool downcase_flag = false);
//! read a tree from a C string
tree* readtree(const char* str, bool downcase_flag = false);

template <typename TreePtrs>
void read_filenamefile_trees(const char* filenamefile, TreePtrs& trees, 
			     tree* (*reader)(FILE*, bool) = &readtree_root, 
			     bool downcase_flag = false)
{
  std::ifstream is(filenamefile);
  if (!is) {
    std::cerr << "Error in tree.h read_filenamefile_trees(): can't open filenamefile " 
	      << filenamefile << std::endl;
    abort();
  }

  std::string filename;
  while (is >> filename) {
    FILE* fp = fopen(filename.c_str(), "r");
    if (fp == NULL) {
      std::cerr << "Error in tree.h read_filenamefile_trees(): can't open filename " 
		<< filename << std::endl;
      abort();
    }

    readtree_filename = filename.c_str();
    readtree_lineno = 1;

    tree* tp;
    while ((tp = reader(fp, downcase_flag)))
      trees.push_back(tp);

    fclose(fp);
  }
}  // read_filenamefiles_trees()


template<typename Proc>
void map_filenamefile_trees(const char* filenamefile, Proc& proc, bool downcase_flag = false)
{
  std::ifstream is(filenamefile);
  if (!is) {
    std::cerr << "Error in tree.h read_tree_file(): can't open filenamefile " 
	      << filenamefile << std::endl;
    abort();
  }

  std::string filename;
  while (is >> filename) {
    FILE* fp = fopen(filename.c_str(), "r");
    if (fp == NULL) {
      std::cerr << "Error in tree.h read_tree_file(): can't open filename " 
		<< filename << std::endl;
      abort();
    }

    readtree_filename = filename.c_str();
    readtree_lineno = 1;

    std::auto_ptr<tree> tp;
    while ((tp = std::auto_ptr<tree>(readtree_root(fp, downcase_flag))).get())
      proc(tp.get());

    fclose(fp);
  }
}


template <typename proc_type>
void map_regex_trees(const char* filename_regex, proc_type& proc, bool downcase_flag = false)  
{
  std::string command("ls ");
  command += filename_regex;
  FILE* filenamefp = popen(command.c_str(), "r");
  int nread;
  char filename[1024];						 // NOTE max filename length
  while((nread = fscanf(filenamefp, "%1024s", filename)) == 1) { // NOTE here too!
    readtree_filename = filename;
    readtree_lineno = 1;
    FILE* fp = fopen(filename, "r");
    assert(fp);

    while (const tree* tp = readtree_root(fp, downcase_flag)) {
      proc(tp);
      delete tp;
    }

    fclose(fp);
  }
  pclose(filenamefp);
}  // map_regex_trees()


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                         tree writing routines                         //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

template <typename label_type>
static void write_tree(std::ostream& s, const tree_node<label_type>* t)
{
  assert(t);

  if (t->child) {
    s << '(' << t->label;
    
    for (tree_node<label_type> *p = t->child; p; p = p->next) {
      s << ' ';
      write_tree(s, p);
    }

    s << ')';
  }
  else 
    s << t->label;
}

template <typename label_type>
static void write_tree_root(std::ostream& s, const tree_node<label_type>* t)
{
  assert(t);
  assert(t->label.is_root());

  if (t->child) {
    s << '(' << t->label;
    
    for (tree_node<label_type> *p = t->child; p; p = p->next) {
      s << ' ';
      write_tree(s, p);
    }

    s << ')';
  }
  else 
    s << t->label;
}

template <typename label_type>
std::ostream& operator<< (std::ostream& os, const tree_node<label_type>* t) {
  if (t)
    write_tree(os, t);
  else
    os << "_NULL_";
  return os;
}  // operator<< ()


template <typename label_type>
std::ostream& operator<< (std::ostream& os, const tree_node<label_type>& t) {
  write_tree(os, &t);
  return os;
}  // operator<< ()


template <typename label_type>
void write_tree_noquote(std::ostream& s, const tree_node<label_type>* t)
{
  assert(t);

  if (t->child) {
    s << '(' << t->label.cat.string_reference();
    
    for (tree_node<label_type> *p = t->child; p; p = p->next) {
      s << ' ';
      write_tree_noquote(s, p);
    }

    s << ')';
  }
  else 
    s << t->label.cat.string_reference();
}

template <typename label_type>
void write_tree_noquote_root(std::ostream& s, const tree_node<label_type>* t)
{
  assert(t);
  assert(t->label.is_root());

  if (t->child) {
    s << '(' << t->label.cat.string_reference();
    
    for (tree_node<label_type> *p = t->child; p; p = p->next) {
      s << ' ';
      write_tree(s, p);
    }

    s << ')';
  }
  else 
    s << t->label.cat.string_reference();
}

template <typename label_type>
int get_head_num(const tree_node<label_type>* t, bool syntactic_head)
{
  assert(t);

  if(t->is_preterminal())
    return 0;

  const tree_node<label_type>* head_child = syntactic_head ? tree_syntacticHeadChild(t): tree_semanticHeadChild(t);

  if (!head_child)
    return 0;

  int head_num = 0;
  tree_node<label_type> *p = t->child;
  while(p != head_child)
    {
      p = p->next;
      ++head_num;
    }

  return head_num;
}

template <typename label_type>
void write_tree_noquote_root_with_heads(std::ostream& s, const tree_node<label_type>* t, bool syntactic_heads)
{
  assert(t);
  assert(t->label.is_root());

  if (t->child) {

    s << '(' << t->label.cat.string_reference() << " H:" << get_head_num(t, syntactic_heads);
    
    for (tree_node<label_type> *p = t->child; p; p = p->next) {
      s << ' ';
      write_tree_with_heads(s, p, syntactic_heads);
    }

    s << ')';
  }
  else 
    s << t->label.cat.string_reference();
}

template <typename label_type>
static void write_tree_with_heads(std::ostream& s, const tree_node<label_type>* t, bool syntactic_heads)
{
  assert(t);

  if (t->child) {

    s << '(' << t->label << " H:" << get_head_num(t, syntactic_heads);
    
    for (tree_node<label_type> *p = t->child; p; p = p->next) {
      s << ' ';
      write_tree_with_heads(s, p, syntactic_heads);
    }

    s << ')';
  }
  else 
    s << t->label;
}

template <typename label_type>
void write_prolog_label(std::ostream& os, const label_type& l) 
{
  const char* c_str = l.cat.c_str();
  assert(c_str != NULL);
  bool needs_quote = !islower(c_str[0]);
  for (const char* cp = c_str; *cp != '\0' && !needs_quote; ++cp) 
    if (!isalnum(*cp) && *cp != '_') 
      needs_quote = true;
  if (needs_quote) {
    os << '\'';
    for (const char* cp = c_str; *cp != '\0'; ++cp) {
      if (*cp == '\'')
	os << "\\'";
      else
	os << *cp;
    }
    os << '\'';
  }
  else  // !needs_quote
    os << c_str;
} // write_prolog_label()

template <typename label_type>
void write_prolog_tree_(std::ostream& fp, const tree_node<label_type>* t)
{
  write_prolog_label(fp, t->label);
  if (t->child && t->child->child == NULL && t->child->next == NULL) {
    fp << "-";
    write_prolog_label(fp, t->child->label);
  }
  else if (t->child) {
    fp << "/[";
    
    for (tree* p = t->child; p; p = p->next) {
      write_prolog_tree_(fp, p);
      
      if (p->next)
	fp << ',';
    }

    fp << ']';
  }
}

template <typename label_type>
void write_prolog_tree(std::ostream& fp, const tree_node<label_type>* t)
{
  write_prolog_tree_(fp, t);
  fp << '.' << std::endl;
}


template <typename label_type>
void display_tree(std::ostream& fp, const tree_node<label_type>* t, int indent=0)
{
  if (t->child) {
    fp << '(' << t->label << ' ';
    { 
      std::ostringstream os;
      os << t->label;
      indent += os.str().size() + 2;  // for '(' and ' '
    }
    display_tree(fp, t->child, indent);

    for(tree_node<label_type>* p = t->child->next; p; p = p->next) {
      fp << '\n';
      for (int i=0; i<indent; i++)
	fp << ' ';
      display_tree(fp, p, indent);
    }
    fp << ')';
  }
  else 
    fp << t->label;
}


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                         precision and recall                          //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

//! precrec_type{} calculates standard parseval precision and recall scores.
//! It uses bag-type evaluation.
//
struct precrec_type {
  unsigned int ncommon;     // number of edges in common
  unsigned int ngold;       // number of edges in gold trees
  unsigned int ntest;       // number of edges in test trees

  precrec_type(unsigned int ncommon = 0, unsigned int ngold = 0, unsigned int ntest = 0) 
    : ncommon(ncommon), ngold(ngold), ntest(ntest) { }

  float precision() const {
    return (ntest == 0) ? 0 : float(ncommon)/ntest;
  }

  float recall() const {
    return (ngold == 0) ? 1 : float(ncommon)/ngold;
  }

  float f_score() const {
    return (ntest == 0 && ngold == 0) ? 0 : (2.0 * ncommon) / (ntest+ngold);
  } // precrec_type::f_score()

  float error_rate() const {
    return (ngold + ntest - 2.0*ncommon) / ngold;
  } // precrec_type::error_rate()

  typedef std::pair<unsigned int,unsigned int> II;
  typedef std::pair<II,symbol> IIS;

  struct edge : public IIS {
    edge(unsigned int left, unsigned int right, symbol cat) : IIS(II(left,right),cat) { }

    unsigned int left() const { return first.first; }
    unsigned int right() const { return first.second; }
    symbol cat() const { return second; }
  };

  struct edges : public std::map<edge, unsigned int> {
    edges() { }
    template <typename Tree>
    edges(const Tree* t) { tree_nontermedges(t, *this); }

    unsigned int nedges() const {
      unsigned int n = 0;
      for (const_iterator it = begin(); it != end(); ++it)
	n += it->second;
      return n;
    }  // precrec_type::edges::nedges()

  };  // precrec_type::edges{}


  //! relabel_category() relabels PRT to ADVP for prec rec evaluation
  //
  inline static symbol relabel_category(symbol cat) {
    static const tree_label::cat_type advp("ADVP"), prt("PRT");    
    return cat == prt ? advp : cat;
  }

  //! tree_nontermedges() maps a tree to its nonterminal edges, ignoring punctuation
  //! and empty nodes as described in the EVALB documentation.
  //
  template <typename Tree>
  static unsigned int tree_nontermedges(const Tree* t, edges& es, 
					unsigned int left = 0, 
					bool nonrootnode = false) {

    static const tree_label::catset_type punctuation(", : `` '' .");

    assert(t != NULL);
    assert(t->child != NULL);

    if (punctuation.contains(t->label.cat) || t->is_none())
      return left;                        // ignore punctuation and empty nodes
  
    if (t->is_preterminal())
      return left+1;                      // preterminal node
  
    unsigned int right = left;
    for (const Tree* c = t->child; c; c = c->next) 
      right = tree_nontermedges(c, es, right, true);

    if (nonrootnode && right > left)      // ignore root node and empty nodes
      ++es[edge(left, right, relabel_category(t->label.cat))];
    
    return right;
  }  // precrec_type::tree_nontermedges()

  // operator() scores two sets of edges and accumulates the scores.
  //
  precrec_type& operator()(const edges& goldedges, const edges& testedges) {
    edges::const_iterator git = goldedges.begin(), tit = testedges.begin();
    while (git != goldedges.end() && tit != testedges.end()) 
      if (git->first == tit->first) { // gold and test edges match
	ncommon += std::min(git->second, tit->second);
	ngold += git++->second;      
	ntest += tit++->second;
      }
      else if (git->first < tit->first) 
	ngold += git++->second;
      else 
	ntest += tit++->second;
    while (git != goldedges.end()) 
      ngold += git++->second;
    while (tit != testedges.end()) 
      ntest += tit++->second;
    return *this;
  }  // precrec_type::operator()

  template <typename Tree>
  precrec_type& operator()(const edges& goldedges, const Tree* testtree) {
    edges testedges(testtree);
    return operator()(goldedges, testedges);
  }  // precrec_type::operator()

  template <typename Tree>
  precrec_type& operator()(const Tree* goldtree, const edges& testedges) {
    edges goldedges(goldtree);
    return operator()(goldedges, testedges);
  }  // precrec_type::operator()

  // operator() scores the two trees and accumulates the scores.
  //
  template <typename Tree1, typename Tree2>
  precrec_type& operator()(const Tree1* goldtree, const Tree2* testtree) {
    edges goldedges(goldtree), testedges(testtree);
    return operator()(goldedges, testedges);
  }  // precrec_type::operator()

  // more initializers that accumulates the results directly
  //
  template <typename Tree>
  precrec_type(const Tree* goldtree, const Tree* testtree) 
    : ncommon(0), ngold(0), ntest(0) {
    operator()(goldtree, testtree);
  }  // precrec_type::precrec_type()

  precrec_type(const edges& goldedges, const edges& testedges) 
    : ncommon(0), ngold(0), ntest(0) {
    operator()(goldedges, testedges);
  }  // precrec_type::precrec_type()

  precrec_type& operator+=(const precrec_type& y) {
    ncommon += y.ncommon;
    ngold += y.ngold;
    ntest += y.ntest;
    return *this;
  }  // operator+=

  void clear() {
    ncommon = ntest = ngold = 0;
  }  // precrec_type::clear()

};  // precrec_type{}


inline std::ostream& operator<< (std::ostream& os, const precrec_type& pr) {
  os << "precision = " << pr.ncommon << '/' << pr.ntest << " = " << pr.precision()
     << ", recall = " << pr.ncommon << '/' << pr.ngold << " = " << pr.recall()
     << ", f-score = " << pr.f_score() << ", error rate = " << pr.error_rate();
  return os;
}

inline bool operator< (const precrec_type& pr1, const precrec_type& pr2) {
  return pr1.f_score() < pr2.f_score();
}

#endif  // TREE_H
