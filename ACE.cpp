/*
 *  © 2013. https://github.com/0xcb All rights reserved.
 *
 *
 *  Usage example:
 *
 *  // Create the ACE object:
 *  ACE* Ace = new ACE();
 *
 *  // Add hundreds or thousands of patterns from byte arrays:
 *  Ace->AddPattern(new ACEPattern(NEEDLE, NEEDLE_SIZE));
 *  Ace->AddPattern(new ACEPattern(ANOTHER_NEEDLE, NEEDLE_SIZE));
 *
 *  // Compile the trie:
 *  Ace->Compile();
 *
 *  // Search a buffer for instances of all patterns in the trie:
 *  Ace->Search(HAYSTACK, HAYSTACK_SIZE);
 *
 *  // Iterate patterns in the trie to find matches:
 *  for(int i = 0; i < Ace->patterns.size(); i++)
 *      if(Ace->patterns[i]->hits)
 *          ; // A pattern was matched! Do something here.
 *
 */

//#pragma once
#pragma intrinsic(memset, memcpy)

#include <vector>
#include <queue>
#include <tuple>
//#include "ACE.h"
#include "ACEPattern.h"

#define BOOL bool

using std::vector;
using std::queue;
using std::tuple;
using std::make_tuple;
using std::get;

class ACE
{

    typedef unsigned char       byte;
    typedef unsigned int        uint;
    typedef unsigned short      ushort;
    typedef unsigned long       ulong;
    typedef vector<ACEPattern*> PatternList;

    private:

        struct INode;
        struct StateNode;
        struct ListNode;

        static const int ALPHABET_SIZE = 256;

        StateNode* _root;

    public:
        
        PatternList patterns;
		vector<std::string> results;
    private:

        /*
         *  Ordering is significant: STATE is TRUE, LIST is FALSE.
         */
        enum NodeType : byte
        {
            NODE_TYPE_LIST  = 0,
            NODE_TYPE_STATE = 1
        };

        /*
         *  Inlined casting functions are used in place of function-style casts because
         *  the Visual Studio compiler disallows them in some sections of code.
         */
        __forceinline static StateNode* STATE(INode* node) { return (StateNode*)node; }
        __forceinline static ListNode*  LIST(INode* node)  { return (ListNode*)node; }


        /*
         *  INode is the underlying type for StateNodes and ListNodes, but also contains
         *  a number of type-agnostic methods that abstract away the internal differences
         *  between the two behind a shared interface.
         */
        #pragma region INode
        
        struct INode
        {
            NodeType type;
            INode*   nextNode[ALPHABET_SIZE];

            __forceinline BOOL IsStateNode()            { return type; }
            __forceinline BOOL IsListNode()             { return !type; }
            __forceinline bool IsTerminal(ushort index) { return IsStateNode() || LIST(this)->size == index; }

            /*
             *  Type-agnostic methods abstract away the complexity of designing the core
             *  trie construction, fail links, and search loop for multiple node types.
             *  These methods are included in the base class rather than invoked
             *  polymorphically because:
             *
             *      a) The IsTerminal case cannot be handled by polymorphism alone (a
             *         ListNode qualifies as terminal at its final index).
             *
             *      b) The methods are inlined for performance reasons, which the
             *         compiler can only do for virtual methods when the type of the
             *         derived class is known at compile time (here it isn't).
             */
            #pragma region Type-agnostic methods
            
            /*
             *  Whether this node has a child with the appropriate byte value.
             */
            __forceinline BOOL HasChild(byte value, ushort index)
            {
                if(IsTerminal(index))
                    return BOOL(nextNode[value]);
                else
                    return LIST(this)->nodes[index + 1]->value == value;
            }

            /*
             *  Only called when a next is guaranteed to exist. No safety check included.
             *  Deprecated in favor of the version below.
             */
            //__forceinline INode* GetChild(byte value, ushort& index)
            //{
            //    if(IsTerminal(index))
            //    {
            //        if(LIST(nextNode[value])->IsListNode())
            //            index = 0;
            //        return nextNode[value];
            //    }
            //    else
            //    {
            //        index++;
            //        return this;
            //    }
            //}

            /*
             *  This version returns NULL if no child node exists for the given byte.
             */
            __forceinline INode* GetChild(byte value, ushort& index)
            {
                if(IsTerminal(index))
                {
                    if(BOOL(nextNode[value]))
                    {
                        if(LIST(nextNode[value])->IsListNode())
                            index = 0;
                        return nextNode[value];
                    }
                    return NULL;
                }
                else
                {
                    if(LIST(this)->nodes[index + 1]->value == value)
                    {
                        index++;
                        return this;
                    }
                    return NULL;
                }
            }

            /*
             *  Get the byte represented by this node. Only call on valid indices (no safety check).
             */
            __forceinline byte GetValue(ushort index)
            {
                if(IsStateNode())
                    return STATE(this)->value;
                else
                    return LIST(this)->nodes[index]->value;
            }

            /*
             *  Get the fail link for this node. Modifies the value of index to track ListNode offset.
             */
            __forceinline INode* GetFailNode(ushort& index)
            {
                if(IsStateNode())
                {
                    // _root is a special case: NULL fail.
                    if(!STATE(this)->failNode)
                        return NULL;

                    if(STATE(this)->failNode->IsListNode())
                        index = STATE(this)->index;
                    return STATE(this)->failNode;
                }
                else
                {
                    auto temp = LIST(this)->nodes[index];
                    index = temp->index;
                    return temp->failNode;
                }
            }

            /*
             *  Assign the fail link for this node.
             */
            __forceinline void SetFailNode(ushort nIndex, INode* fail, ushort fIndex)
            {
                if(IsStateNode())
                {
                    STATE(this)->index    = fIndex;
                    STATE(this)->failNode = fail;
                }
                else
                {
                    LIST(this)->nodes[nIndex]->index    = fIndex;
                    LIST(this)->nodes[nIndex]->failNode = fail;
                }
            }

            /*
             *  The list of patterns matched by reaching this node.
             */
            __forceinline PatternList* GetMatches(ushort index)
            {
                if(IsStateNode())
                    return STATE(this)->matches;
                else
                    return LIST(this)->nodes[index]->matches;
            }

            /*
             *  If the node already has a PatternList, a new one is not created.
             */
            __forceinline PatternList* CreateMatches(ushort index)
            {
                auto matches = GetMatches(index);
                if(!matches)
                {
                    matches = new PatternList();
                    if(IsStateNode())
                        STATE(this)->matches = matches;
                    else
                        LIST(this)->nodes[index]->matches = matches;
                }
                return matches;
            }

            /*
             *  Used to copy matches depth-ward in the trie while establishing fail links.
             */
            __forceinline void CopyMatches(ushort nIndex, INode* copy, ushort cIndex)
            {
                auto list = copy->GetMatches(cIndex);
                if(!list)
                    return;

                PatternList* matches = CreateMatches(nIndex);
                for(uint i = 0; i < list->size(); i++)
                    matches->push_back((*list)[i]);
            }

            /*
             *  The number of child nodes descending from this node. Not optimal (could be
             *  cached) but also not used in performance-critical areas.
             */
            uint GetChildCount()
            {
                uint count = 0;
                for(uint i = 0; i < ALPHABET_SIZE; i++)
                    if(nextNode[i])
                        count++;
                return count;
            }

            #pragma endregion

            INode()
            {
                memset(nextNode, 0, sizeof(INode*) * ALPHABET_SIZE);
            }

            virtual ~INode()
            {
                // Twiddling thumbs.
            }
        };

        #pragma endregion

        
        /*
         *  StateNodes are the standard node type seen in other Aho-Corasick
         *  implementations. They contain only one state.
         */
        #pragma region StateNode

        struct StateNode : public INode
        {
            byte         value;
            INode*       failNode;
            ushort       index;
            PatternList* matches;

            StateNode(byte val)
            {
                type    = NODE_TYPE_STATE;
                value   = val;
                index   = 0;
                matches = NULL;
            }

            ~StateNode()
            {
                if(matches)
                    delete matches;
            }
        };

        #pragma endregion


        /*
         *  ListNodes contain a series of nodes with only one state transition each. This
         *  is possible because only a very small percentage of nodes have more than one
         *  state transition, and allows us to save a significant amount of memory over
         *  standard StateNodes (alphabet size * pointer size per state, or 2kB per state
         *  on 64-bit).
         */
        #pragma region ListNode

        struct ListNode : public INode
        {
            /*
             *  NodeEntry is the ListNode equivalent of a single state.
             */
            #pragma region NodeEntry

            struct NodeEntry
            {
                byte         value;
                INode*       failNode;
                ushort       index;
                PatternList* matches;

                NodeEntry()
                {
                    index = 0;
                }

                ~NodeEntry()
                {
                    if(matches)
                        delete matches;
                }
            };

            #pragma endregion

            // Size is cached to avoid the overhead of repeated calls to the size()
            // method in the Search loop.
            ushort             size;
            vector<NodeEntry*> nodes;

            /*
             *  Add a new state transition from a byte value.
             */
            void AddValue(byte value)
            {
                auto entry     = new NodeEntry;
                entry->value   = value;
                entry->matches = NULL;
                nodes.push_back(entry);
                size = nodes.size() - 1;
            }

            /*
             *  Split an existing list into two lists and connect them. Return a
             *  pointer to the new list.
             */
            INode* Split(ushort index)
            {
                // There is no bounds check on index because it is impossible to
                // overflow with the current code.

                auto list = new ListNode(nodes[index]);
                for(ushort i = index + 1; i <= size; i++)
                    list->nodes.push_back(nodes[i]);
                nodes.erase(nodes.begin() + index, nodes.end());

                memcpy(list->nextNode, nextNode, ALPHABET_SIZE * sizeof(INode*));
                memset(nextNode, 0, ALPHABET_SIZE * sizeof(INode*));
                nextNode[list->nodes[0]->value] = list;

                list->size = list->nodes.size() - 1;
                size       = nodes.size() - 1;

                return list;
            }

            /*
             *  Is the list composed of a single NodeEntry?
             */
            __forceinline bool IsSingleState()
            {
                return !LIST(this)->size;
            }

            /*
             *  Reduce the ListNode to a StateNode with values from the ListNode's
             *  first NodeEntry. Only called if IsSingleState is true.
             */
            StateNode* ReduceToState(INode* parent)
            {
                auto entry = LIST(this)->nodes[0];
                auto state = new StateNode(entry->value);
                if(entry->matches)
                    state->matches = new PatternList(*entry->matches);
                parent->nextNode[entry->value] = state;
                memcpy(state->nextNode, nextNode, ALPHABET_SIZE * sizeof(INode*));
                delete this;
                return state;
            }

            ListNode(byte value)
            {
                type = NODE_TYPE_LIST;
                AddValue(value);
            }

            ListNode(NodeEntry* entry)
            {
                type = NODE_TYPE_LIST;
                nodes.push_back(entry);
            }

            ~ListNode()
            {
                size++;
                for(uint i = 0; i < size; i++)
                    delete nodes[i];
            }
        };

        #pragma endregion


        void CompressVectors(INode* node)
        {
            if(node->IsStateNode())
            {
                if(STATE(node)->matches)
                    STATE(node)->matches->shrink_to_fit();
            }
            else
            {
                LIST(node)->nodes.shrink_to_fit();
                uint size = LIST(node)->size + 1;
                for(uint i = 0; i < size; i++)
                    if(LIST(node)->nodes[i]->matches)
                        LIST(node)->nodes[i]->matches->shrink_to_fit();
            }

            for(uint i = 0; i < ALPHABET_SIZE; i++)
                if(node->nextNode[i])
                    CompressVectors(node->nextNode[i]);
        }

    public:

        ACE()
        {
            _root           = new StateNode((byte)NULL);
            _root->failNode = NULL;
        }

        void AddPattern(ACEPattern* pattern)
        {
            patterns.push_back(pattern);
        }

        void Compile()
        {
            /*
             *  Create the trie using ListNodes as the default. If branching occurs,
             *  split the ListNode appropriately.
             */
            #pragma region Construct the trie

            uint size = patterns.size();
            for(uint i = 0; i < size; i++)
            {
                INode* node  = _root;
                ushort index = 0;
                for(uint j = 0; j < patterns[i]->length; j++)
                {
                    byte next = patterns[i]->pattern[j];
                    if(!node->HasChild(next, index))
                    {
                        // Necessary because _root is a StateNode.
                        if(node->IsStateNode())
                            node->nextNode[next] = new ListNode(next);
                        else
                        {
                            BOOL isListEnd   = index == LIST(node)->size;
                            BOOL hasChildren = node->GetChildCount();
                            if(isListEnd && !hasChildren)
                                LIST(node)->AddValue(next);
                            else
                            {
                                if(!isListEnd)
                                    LIST(node)->Split(index + 1);
                                node->nextNode[next] = new ListNode(next);
                            }
                        }
                    }
                    node = node->GetChild(next, index);
                }
                node->CreateMatches(index)->push_back(patterns[i]);
            }

            #pragma endregion

            /*
             *  Fail links are more complicated with ListNodes, but most of the
             *  thorniness has been abstracted into type-agnostic methods in INode.
             */
            #pragma region Create failure links
            
            queue<tuple<INode*, ushort, INode*, ushort>> nodeQueue;
            nodeQueue.push(make_tuple(_root, 0, STATE(NULL), 0));
            while(!nodeQueue.empty())
            {
                auto data   = nodeQueue.front();
                auto node   = get<0>(data);
                auto nIndex = get<1>(data);
                auto parent = get<2>(data);
                auto fIndex = get<3>(data);
                auto value  = node->GetValue(nIndex);
                auto fail   = !parent ? NULL : parent == _root ? _root : parent->GetFailNode(fIndex);

                // Surely there's a cleaner way to exclude _root.
                if(fail)
                {
                    // All non-root nodes are ListNodes at the time they are evaluated here. If
                    // they have only one state, we reduce them to a StateNode.
                    if(LIST(node)->IsSingleState())
                        node = LIST(node)->ReduceToState(parent);

                    while(!fail->HasChild(value, fIndex) && (fail = fail->GetFailNode(fIndex)));
                    node->SetFailNode(nIndex, parent == _root || !fail ? _root : fail->GetChild(value, fIndex), fIndex);
                    
                    // Copy fail link matches depth-ward in the trie.
                    fail = node->GetFailNode(fIndex = nIndex);
                    node->CopyMatches(nIndex, fail, fIndex);
                }

                // Queue children. A "terminal" node is either a StateNode or the last entry
                // in a ListNode.
                if(node->IsTerminal(nIndex))
                    for(uint i = 0; i < ALPHABET_SIZE; i++)
                    {
                        if(node->nextNode[i])
                            nodeQueue.push(make_tuple(node->nextNode[i], 0, node, nIndex));
                    }
                else
                    nodeQueue.push(make_tuple(node, nIndex + 1, node, nIndex));

                nodeQueue.pop();
            }

            #pragma endregion

            // Recurse through all node vectors and free excess space. Not that it amounts
            // to much.
            CompressVectors(_root);
        }

        /*
         *  The abstracted INode methods leave us with a very compact search loop.
         */
        void Search(byte* buffer, uint length)
        {
			results.clear();
            INode* node  = _root;
            ushort index = 0;
            for(uint i = 0; i < length; i++)
            {
                byte next = buffer[i];
                INode* temp;
                while(!(temp = node->GetChild(next, index)) && (node = node->GetFailNode(index)));
                node = node ? temp : _root;

                auto matches = node->GetMatches(index);
                if(matches)
                {
                    uint size = matches->size();
                    for(uint j = 0; j < size; j++)
                    {
                        auto matched = (*matches)[j];
                        /*
                        if(!matched->hits)
                            matched->hits = new vector<ulong>;
                        matched->hits->push_back(i - (matched->length - 1));
                        */
                        results.push_back(std::string((const char*)matched->pattern, matched->length));
                    }
                }
            }
        }

    private:

        void DeleteNodes(INode* node)
        {
            for(uint i = 0; i < ALPHABET_SIZE; i++)
                if(node->nextNode[i])
                    DeleteNodes(node->nextNode[i]);
            delete node;
        }


    public:

        ~ACE()
        {
            DeleteNodes(_root);
            for(uint i = 0; i < patterns.size(); i++)
                delete patterns[i];
        }
};
