//
// Created by colin on 11/9/2018.
//

#pragma once

#include <stdint.h>

#include "binbag.h"


namespace Rest {

    template<class TNode, class TLiteral, class TArgumentType>
    class Pool
    {
    protected:
        // stores the expression as a chain of endpoint nodes
        // todo: this will need to use paged memory
        TNode *ep_head, *ep_tail, *ep_end;
        size_t sznodes;

        // allocated text strings
        binbag* text;

    public:
        Pool() : sznodes(32), text( binbag_create(128, 1.5) )
        {
            ep_head = ep_tail =  (TNode*)calloc(sznodes, sizeof(TNode));
            ep_end = ep_head + sznodes;
        }

        virtual ~Pool() {
            ::free(ep_head);
            binbag_free(text);
        }

        TNode* newNode()
        {
            assert(ep_tail < ep_end);
            return new (ep_tail++) TNode();
        }

        TArgumentType* newArgumentType(const char* name, unsigned short typemask)
        {
            // todo: make this part of paged memory
            long nameid = binbag_insert_distinct(text, name);
            return new TArgumentType(binbag_get(text, nameid), typemask);  // todo: use our binbag here
        }

        long findLiteral(const char* word) {
            return binbag_find_nocase(text, word);
        }

        TLiteral* newLiteral(TNode* ep, TLiteral* literal)
        {
            TLiteral* _new, *p;
            int _insert;
            if(ep->literals) {
                // todo: this kind of realloc every Literal insert will cause memory fragmentation, use Endpoints shared mem
                // find the end of this list
                TLiteral *_list = ep->literals;
                while (_list->isValid())
                    _list++;
                _insert = (int)(_list - ep->literals);

                // allocate a new list
                _new = (TLiteral*)realloc(ep->literals, (_insert + 2) * sizeof(TLiteral));
                //memset(_new+_insert+1, 0, sizeof(Literal));
            } else {
                _new = (TLiteral*)calloc(2, sizeof(TLiteral));
                _insert = 0;
            };

            // insert the new literal
            memcpy(_new + _insert, literal, sizeof(TLiteral));
            ep->literals = _new;

            p = &_new[_insert + 1];
            p->id = -1;
            p->isNumeric = false;
            p->next = nullptr;

            return _new + _insert;
        }


        TLiteral* addLiteralString(TNode* ep, const char* literal_value)
        {
            TLiteral lit;
            lit.isNumeric = false;
            if((lit.id = binbag_find_nocase(text, literal_value)) <0)
                lit.id = binbag_insert(text, literal_value);  // insert value into the binbag, and record the index into the id field
            lit.next = nullptr;
            return newLiteral(ep, &lit);
        }

        TLiteral* addLiteralNumber(TNode* ep, ssize_t literal_value)
        {
            TLiteral lit;
            lit.isNumeric = true;
            lit.id = literal_value;
            lit.next = nullptr;
            return newLiteral(ep, &lit);
        }


    };

}
