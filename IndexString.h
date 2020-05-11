#ifndef INDEXSTRING_H_INCLUDED
#define INDEXSTRING_H_INCLUDED

/*
    [Valverde Computing copyright notice]

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    As an exception, the copyright holders of this Library grant you permission to
    (i) compile an Application with the Library, and
    (ii) distribute the Application containing code generated by the Library and
    added to the Application during this compilation process under terms of your choice,
    provided you also meet the terms and conditions of the Application license.

IndexString.h - header file for Kaldane IndexString string, which are variable-length (with an upper
  bound) null-terminated byte strings that have their indexes and a "poor man's normalized key"
  (pmnk) moved when they are swapped, without moving their tail strings (ignoring the Sort Benchmark
  rules). IndexString has "value-string semantics" as Stroustrup defined for his String type (C++11,
  chapter 19.3) IndexString strings behave like Direct strings for short lengths (16 bytes or 
  less, by internally equating the pmnk length with the string length.) This allows IndexString 
  strings to match Direct strings in performance for short strings and still have constant time 
  complexity for any string length: they are a candidate for general purpose string programming 
  with stack slab allocation/deallocation as opposed to the fine-grained allocators, which are 
  necessary for pointer strings like std::string. IndexString strings isolate the head (index + 
  pmnk) from the tail, which deploys a Direct string with a typedef called "tail". At scale, 
  IndexString strings sort faster than Direct or Symbiont strings and much, much faster than 
  std::strings, which require much more space as well (probably due to fine-grained allocation.) 
  The differences are smaller for short strings but become more pronounced for longer strings and  
  especially for merge sorts, where std:string is quadratic in the release build. Both merge and 
  quick sorts are effectively linear for IndexString strings and constant for string length. 
  IndexString strings, like Symbiont strings are quadratic in the debug build for  merge sort, so 
  remember to use the release build for performance analysis. They are designed to be used  with 
  with slab allocation/deallocation on the stack, as opposed to fine-grained allocators. Since the 
  two allocated slabs never need to contain pointers, only array indices, the slab data structures 
  are base+offset and can be mmapped to a file or /dev/shm and shared locally or across a memory 
  fabric such as Gen-Z, or stored and transmitted, or mmapped over an NFS: consistency 
  considerations for NFS are an issue for sharing, of course (caveat participem).
  
  The name Kaldane is an antique SciFi reference (heads move, bodies stay):
  http://www.catspawdynamics.com/images/gino_d%27achille_5-the_chessmen_of_mars.jpg
  or google image search the word "kaldane"
  https://en.wikipedia.org/wiki/Kaldane
  
  IndexString class (variation of Head) in simple arrays for indexes on its RowString friend class

  As stated before, the IndexString class starts with the identical template parameters as the 
  RowString class table that it creates an index for. In addition, IndexString has a separate 
  tuning optional template parameter named pmnkSize, which defaults to 7, the sweet spot for 
  well-behaved strings. Both IndexString and RowString carry their internals in a struct, because 
  that makes copying and sizing operations more convenient, and the sizes of those are in 
  increments of 4 bytes from C++.
  
  Poor Man’s Normalized Keys (pmnk)
  
  The IndexString struct contains the 4-byte K-value as an index into the array of IndexString 
  objects at the front, and then the char pmnk[pmnkSize + 1] null terminated string that is the 
  head, of all of the column strings inside the RowString that are being indexed. That means that 
  pmnkSize should logically have values of 3, 7, 11, 15… for strings without lots of repeating 
  characters in the front (e.g., “           Joseph          Cotton“) and that have a somewhat 
  random distribution, 7 is a good average size and that keeps the system from having to jump to 
  the rest of the string kept in the RowString column, because the pmnk kept in the IndexString 
  was too short to satisfy some comparison.
  
  As is stated in Goetz Graefe’s paper: B-tree indexes and CPU Caches, Goetz Graefe and Per-Ake 
  Larson: IEEE Proceedings of the 17th International Conference on Data Engineering Section 6.1, 
  "Poor Man's Normalized Keys", p. 355:
  
	"This technique works well only if common key prefixes are truncated before the poor man's 
    normalized keys are extracted. In that case, a small poor man's normalized key (e.g., 2 bytes) 
    may be sufficient to avoid most cache faults on the full records; if prefix truncation is not 
    used, even a fairly large poor man's normalized key may not be very effective, in particular 
    in B-tree leaves."
  
  It might be cautiously suggested that indexes maintained on data requiring a lot of prefix 
  truncation might have lower value in any case: mining low-grade ore is bound to be less 
  satisfying than mining high-grade ore.
  
  Friend Class, Accessor Member Functions and Dropping the Anchor
  
  At the top of the IndexString class, the template parameters in common with the RowString class 
  template fully-defined type, are used to declare RowString as a friend class. That allows access 
  to all of the RowString table internals that the IndexString index is an accessor for. You could 
  have gotten some limited access with a derived type, but then you would be carrying around the 
  bodies of the RowString in the IndexString elements, so the friend declaration is a saving grace.
  
  To get access to the table row from the index, several accessor functions are added:
  
	•	row(): typed access to the actual row table element that the index element is pointing to.
	•	rowAnchor(): typed access to the base of the table, which the index is pointing to an 
		element of, such that it can be indexed separately (e.g., join linkage on the “from” table).
	•	c_str(): char pointer access into the column of the table row that the index element is 
		pointing to.
  
  All of the comparison operators are supported, both against char strings and other IndexString 
  elements (of the identical type), this allows you to compare different columns within the table, 
  and columns across different tables. There is an ostream<< output function that outputs the 
  RowString table element that the IndexString element is pointing to, but there is no input 
  function: that is supplied by a member function called copyKey() that is only called by the 
  anchor routine below.
  
  The anchor routine for an IndexString index can only be called after the RowString table, that 
  it is indexing, has been fully loaded by one of the three assign member functions, and then has 
  had its own anchor dropped. Calling dropAnchorCopyKeysSortIndex(IndexString indexArr[], 
  std::size_t size) then does the eponymous three things, returning a working index.
  
  Any column of a table can be indexed at any time after the table is built and anchored, but to 
  repeat the critical point, these are automatic variables allocated on the stack in the demo 
  programs, and they will disappear if they are allocated in a procedure and that procedure 
  returns. That is what mmap is for: to manage virtual address space against a large memory system.
  
*/papers/2017/

#include <iostream>
#include <typeinfo>papers/2017/
#include <cstring>
#include "RowString.h"
#include "Sorts.h"

using namespace std;

template <Column columnEnum, 
         // these next 5 parameters must match the RowString parameters,
         // declared for this index, or the friend class below won't match up
         typename T, std::size_t maxStringSize, Table tableEnum, std::size_t maxColumnsCount, std::size_t maxColumnSize,
         std::size_t pmnkSize,
         // The parameter U is the Integer type used for the k base+offset index, which makes
         // a big difference at scale for the space complexity. At enormous scale, greater than
         // 2^32 rows, long long must be used to index them. Using short saves no space due to
         // alignment. Hence U should be int or long long.
         typename U>
class IndexString
{
    friend class RowString<T, maxStringSize, tableEnum, maxColumnsCount, maxColumnSize>;
    static const std::size_t maxPmnkSize = 19;
    static const bool noPmnk = pmnkSize == 0;
    static const std::size_t adjustedPmnkSize
        = noPmnk ? 0 : ((pmnkSize > maxPmnkSize) ? (((maxPmnkSize/4) * 4) + 3) : (((pmnkSize/4) * 4) + 3));
    static const int storagePmnkSize = noPmnk ? -1 : int(adjustedPmnkSize); // -1 makes the pmnk char array zero-length, below.

public:

    // Try to make the PMNK string + null fit in an integer (4-byte) package, struct round-off by the compiler
    // will take that space anyway: Optimal sizes for pmnkSize are 3,7,11,15,19 ..., the default is 7,
    // which is a little slower than 3 at the small scale (when it doesn't matter), but much faster than
    // three at enormous scale, due to the greater number of identical pmnk at that scale.
    struct IndexStringStruct
            // Movable, references each RowString by the column index with a striding offset from the RowString columnOffset array.
    {
        U k;
        char pmnk[storagePmnkSize + 1]; // Poor Man's Normalized Key, see below. Null-terminated.
    };

    /* B-tree Indexes and CPU Caches Goetz Graefe and Per-Ake Larson
        IEEE Proceedings of the 17th International Conference on Data Engineering
        Section 6.1, "Poor Man's Normalized Keys", p. 355:

        This technique works well only if common
        key prefixes are truncated before the poor man's
        normalized keys are extracted. In that case, a small poor
        man's normalized key (e.g., 2 bytes) may be sufficient to
        avoid most cache faults on the full records; if prefix truncation
        is not used, even a fairly large poor man's normalized
        key may not be very effective, in particular in B-tree
        leaves.
    */

    IndexStringStruct h;

public:

    typedef RowString<T, maxStringSize, tableEnum, maxColumnsCount, maxColumnSize> rowType;
    static const int rowAnchorOffset = (int)tableEnum;
    static const int indexAnchorOffset = (int)columnEnum;

    class Item_Size_Mismatch : public runtime_error
    {
    public:
        Item_Size_Mismatch() :
            runtime_error("array[0].structSize() must equal arrayByteLength/arrayCount (possible compiler dependency)") {}
    };

    class Assign_String_Too_Long : public runtime_error
    {
    public:
        Assign_String_Too_Long() :
            runtime_error("assign function called with overlong string") {}
    };

    class Bad_RowString_Anchor : public runtime_error
    {
    public:
        Bad_RowString_Anchor() :
            runtime_error("after allocation, you MUST call rowArray[0].dropAnchor(rowArray, correct_arrayCount)") {}
    };

    class Bad_IndexString_Anchor : public runtime_error
    {
    public:
        Bad_IndexString_Anchor() :
            runtime_error("no index, or corrupted index for this column of  this table") {}
    };

    class Already_IndexString_Anchor : public runtime_error
    {
    public:
        Already_IndexString_Anchor() :
            runtime_error("this index anchor has already been initialized once") {}
    };

    class Over_Writing_Existing_IndexString_Key : public runtime_error
    {
    public:
        Over_Writing_Existing_IndexString_Key() :
            runtime_error("index keys copied from RowString only once during IndexString creation") {}
    };

    class Bad_IndexString_Field_Null_Termination : public runtime_error
    {
    public:
        Bad_IndexString_Field_Null_Termination() :
            runtime_error("index fields MUST BE null terminated within the row array") {}
    };

    class Should_Not_Execute_Here : public runtime_error
    {
    public:
        Should_Not_Execute_Here() :
            runtime_error("program should not execute here (performance debugging)") {}
    };

    constexpr IndexString()
    {
    }

    IndexString(const IndexString& rhs)
    {
        h = rhs.h; // heads only move
    }

    IndexString& operator = (const IndexString& rhs)
    {
        h = rhs.h; // heads only move
        return *this;
    }

    IndexString& copyKey() // Only used during IndexString init: copy the pmnk of the IndexString key from the RowString
    {
        if (charIndexAnchors[indexAnchorOffset] == 0) throw Bad_IndexString_Anchor(); // no index for this column
        if (h.k != 0) throw Over_Writing_Existing_IndexString_Key(); // some confusion as to order of operations
        // First, get the k-value, the corresponding rowString-array-index
        char *pmnkPtr = h.pmnk;
        char *hPtr = pmnkPtr - offsetof(IndexStringStruct, pmnk);
        int delta = hPtr - charIndexAnchors[indexAnchorOffset];
        int k = delta/sizeof(h);
//        int k = ((char*)h.pmnk - offsetof(IndexStringStruct, pmnk) - charAnchors[indexAnchorOffset])/sizeof(h);
        if (noPmnk)
        {
            h.k = k;
            return *this;
        }
        h.pmnk[0] = 0;
        // Second, get the rowString-column string pointer (source)
        char *rowStr = charRowAnchors[rowAnchorOffset] + (sizeof(((rowType*)0)->r) * k); // points to first byte of corresponding row
        rowType *row = (rowType*)(rowStr); // row pointer of corresponding row
        char *columnStr = rowStr + row->r.columnOffset[columnId[indexAnchorOffset]];
        // Third, get the rowString's column-string length
        std::size_t len = strlen(columnStr);
        // Create the IndexString element
        if (len > adjustedPmnkSize)
        {
            len = adjustedPmnkSize;
            memcpy(h.pmnk, columnStr, len);
        }
        else strcpy(h.pmnk, columnStr);
        h.pmnk[len] = 0;
        h.k = k;
        return *this;
    }

    constexpr std::size_t structSize() const
    {
        return sizeof(h);
    }

    constexpr int indexAnchorOff() const
    {
        return indexAnchorOffset;
    }

    constexpr Column enumColumn() const
    {
        return columnEnum;
    }

    constexpr Table enumTable() const
    {
        return tableEnum;
    }

    constexpr std::size_t columnPmnksize() const
    {
        return adjustedPmnkSize;
    }

    constexpr std::size_t count() const
    {
        return indexCounts[indexAnchorOffset];
    }

    constexpr int KValue() const
    {
        return h.k;
    }

    rowType* row()
    {
        if (charRowAnchors[rowAnchorOffset] == 0) throw Bad_RowString_Anchor(); // no table zeroFromColumnto index
        if (charIndexAnchors[indexAnchorOffset] == 0 || h.k < 0)
            throw Bad_IndexString_Anchor(); // no index for this column

        char *rowStr = charRowAnchors[rowAnchorOffset] + (sizeof(((rowType*)0)->r) * h.k); // points to first byte of rhs row
        rowType *row = (rowType*)(rowStr); // rhs row pointer

        return row;
    }

    rowType* rowAnchor()
    {
        if (charRowAnchors[rowAnchorOffset] == 0) throw Bad_RowString_Anchor(); // no table to index

        char *rowStr = charRowAnchors[rowAnchorOffset];
        rowType *row = (rowType*)(rowStr); // row anchor pointer

        return row;
    }

    char* c_str() // returns a more costly pointer into the column inside the row at the IndexStringStruct h.k offset
    {
        if (charRowAnchors[rowAnchorOffset] == 0) throw Bad_RowString_Anchor(); // no table to index
        if (charIndexAnchors[indexAnchorOffset] == 0 || h.k < 0)
            throw Bad_IndexString_Anchor(); // no index for this column

        char *lhsRowStr = charRowAnchors[rowAnchorOffset] + (sizeof(((rowType*)0)->r) * h.k); // points to first byte of lhs row
        rowType *lhsRow = (rowType*)(lhsRowStr); // lhs row pointer
        char *lhsColumnStr = lhsRowStr + lhsRow->r.columnOffset[columnId[indexAnchorOffset]];

        std::size_t len = strlen(lhsColumnStr);
        if (len > maxColumnSize) throw Bad_IndexString_Field_Null_Termination();papers/2017/
        return lhsColumnStr;
    }

private:

    inline int compareTail(const IndexString& rhs)
    {
        if (charRowAnchors[rowAnchorOffset] == 0) throw Bad_RowString_Anchor(); // no table to index
        if (charIndexAnchors[indexAnchorOffset] == 0 || h.k < 0 || rhs.h.k < 0)
            throw Bad_IndexString_Anchor(); // no index for this column
        // throw Should_Not_Execute_Here(); // this is for performance debugging

        char *lhsRowStr = charRowAnchors[rowAnchorOffset] + (sizeof(((rowType*)0)->r) * h.k); // points to first byte of lhs row
        rowType *lhsRow = (rowType*)(lhsRowStr); // lhs row pointer
        char *lhsColumnStr = lhsRowStr + lhsRow->r.columnOffset[columnId[indexAnchorOffset]] + adjustedPmnkSize; // points after adjustedPmnkSize bytes

        char *rhsRowStr = charRowAnchors[rowAnchorOffset] + (sizeof(((rowType*)0)->r) * rhs.h.k); // points to first byte of rhs row
        rowType *rhsRow = (rowType*)(rhsRowStr); // rhs row pointer
        char *rhsColumnStr = rhsRowStr + rhsRow->r.columnOffset[columnId[indexAnchorOffset]] + adjustedPmnkSize; // points after adjustedPmnkSize bytes

        return strcmp(lhsColumnStr, rhsColumnStr);
    }

    inline int compareTail(const char rhsTailStr[])
    {
        if (charRowAnchors[rowAnchorOffset] == 0) throw Bad_RowString_Anchor(); // no table to index
        if (charIndexAnchors[indexAnchorOffset] == 0 || h.k < 0)
            throw Bad_IndexString_Anchor(); // no index for this column
        // throw Should_Not_Execute_Here(); // this is for performance debugging

        char *lhsRowStr = charRowAnchors[rowAnchorOffset] + (sizeof(((rowType*)0)->r) * h.k); // points to first byte of lhs row
        rowType *lhsRow = (rowType*)(lhsRowStr); // lhs row pointer
        char *lhsColumnStr = lhsRowStr + lhsRow->r.columnOffset[columnId[indexAnchorOffset]] + adjustedPmnkSize; // points after adjustedPmnkSize bytes

        return strcmp(lhsColumnStr, rhsTailStr);
    }

public:

// IndexString to IndexString comparisons

    inline bool operator < (const IndexString& rhs)
    {
        if (h.k == rhs.h.k) return false; // pivot, temp or external array identity optimization
        if (noPmnk)
        {
            bool state = (compareTail(rhs) < 0);
            return state;
        }
        int pmnkCompare = strcmp(h.pmnk, rhs.h.pmnk);
        if (pmnkCompare < 0) return true; // independent of the tail
        else if (pmnkCompare > 0 || (pmnkCompare == 0 && strlen(rhs.h.pmnk) < adjustedPmnkSize))
            return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs) < 0);
            return state;
        }
    }
papers/2017/
    inline bool operator <= (const IndexString& rhs)
    {
        if (h.k == rhs.h.k) return true; // pivot, temp or external array identity optimization
        if (noPmnk)
        {
            bool state = (compareTail(rhs) < 1);
            return state;
        }
        int pmnkCompare = strcmp(h.pmnk, rhs.h.pmnk);
        if (pmnkCompare < 0 || (pmnkCompare == 0 && strlen(rhs.h.pmnk) < adjustedPmnkSize))
            return true; // independent of the tail
        else if (pmnkCompare > 0) return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs) < 1);
            return state;
        }
    }

    inline bool operator == (const IndexString& rhs)
    {
        if (h.k == rhs.h.k) return true; // pivot, temp or external array identity optimization
        if (noPmnk)
        {
            bool state = (compareTail(rhs) == 0);
            return state;
        }
        int pmnkCompare = strcmp(h.pmnk, rhs.h.pmnk);
        if (pmnkCompare == 0 && strlen(rhs.h.pmnk) < adjustedPmnkSize)
            return true; // independent of the tail
        else if (pmnkCompare != 0) return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs) == 0);
            return state;
        }
    }

    inline bool operator != (const IndexString& rhs)papers/2017/
    {
        if (h.k == rhs.h.k) return false; // pivot, temp or external array identity optimization
        if (noPmnk)
        {
            bool state = (compareTail(rhs) != 0);
            return state;
        }
        int pmnkCompare = strcmp(h.pmnk, rhs.h.pmnk);
        if (pmnkCompare == 0 && strlen(rhs.h.pmnk) < adjustedPmnkSize)
            return false; // independent of the tail
        else if (pmnkCompare != 0) return true; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs) != 0);
            return state;
        }
    }

    inline bool operator >= (const IndexString& rhs)
    {
        if (h.k == rhs.h.k) return true; // pivot, temp or external array identity optimization
        if (noPmnk)
        {
            bool state = (compareTail(rhs) > -1);
            return state;
        }
        int pmnkCompare = strcmp(h.pmnk, rhs.h.pmnk);
        if (pmnkCompare > 0 || (pmnkCompare == 0 && strlen(rhs.h.pmnk) < adjustedPmnkSize))papers/2017/
            return true; // independent of the tail
        else if (pmnkCompare < 0 ) return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs) > -1);
            return state;
        }
    }

    inline bool operator > (const IndexString& rhs)
    {
        if (h.k == rhs.h.k) return false; // pivot, temp or external array identity optimization
        if (noPmnk)
        {
            bool state = (compareTail(rhs) > 0);
            return state;
        }
        int pmnkCompare = strcmp(h.pmnk, rhs.h.pmnk);
        if (pmnkCompare > 0) return true; // independent of the tail
        else if (pmnkCompare < 0 || (pmnkCompare == 0 && strlen(rhs.h.pmnk) < adjustedPmnkSize))
            return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs) > 0);
            return state;
        }
    }

// IndexString to char[] comparisons

    inline bool operator < (const char rhs[]) // null-terminated string compare
    {
        if (noPmnk)
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) < 0);
            return state;
        }
        int pmnkCompare = strncmp(h.pmnk, rhs, adjustedPmnkSize);
        if (pmnkCompare < 0) return true; // independent of the tail
        else if (pmnkCompare > 0 || (pmnkCompare == 0 && strlen(rhs) < adjustedPmnkSize))
            return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) < 0);
            return state;
        }
    }

    inline bool operator <= (const char rhs[]) // null-terminated string compare
    {
        if (noPmnk)
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) < 1);
            return state;
        }
        int pmnkCompare = strncmp(h.pmnk, rhs, adjustedPmnkSize);
        if (pmnkCompare < 0 || (pmnkCompare == 0 && strlen(rhs) < adjustedPmnkSize))
            return true; // independent of the tail
        else if (pmnkCompare > 0) return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) < 1);
            return state;
        }
    }

    inline bool operator == (const char rhs[]) // null-terminated string compare
    {
        if (noPmnk)
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) == 0);
            return state;
        }
        int pmnkCompare = strncmp(h.pmnk, rhs, adjustedPmnkSize);
        if (pmnkCompare == 0 && strlen(rhs) < adjustedPmnkSize)
            return true; // independent of the tail
        else if (pmnkCompare != 0) return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) == 0);
            return state;
        }
    }

    inline bool operator != (const char rhs[]) // null-terminated string compare
    {
        if (noPmnk)
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) != 0);
            return state;
        }
        int pmnkCompare = strncmp(h.pmnk, rhs, adjustedPmnkSize);
        if (pmnkCompare == 0 && strlen(rhs) < adjustedPmnkSize)
            return false; // independent of the tail
        else if (pmnkCompare != 0) return true; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) != 0);
            return state;
        }
    }

    inline bool operator >= (const char rhs[]) // null-terminated string compare
    {
        if (noPmnk)
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) > -1);
            return state;
        }
        int pmnkCompare = strncmp(h.pmnk, rhs, adjustedPmnkSize);
        if (pmnkCompare > 0 || (pmnkCompare == 0 && strlen(rhs) < adjustedPmnkSize))
            return true; // independent of the tail
        else if (pmnkCompare < 0 ) return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) > -1);
            return state;
        }
    }

    inline bool operator > (const char rhs[]) // null-terminated string compare
    {
        if (noPmnk)
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) > 0);
            return state;
        }
        int pmnkCompare = strncmp(h.pmnk, rhs, adjustedPmnkSize);
        if (pmnkCompare > 0) return true; // independent of the tail
        else if (pmnkCompare < 0 || (pmnkCompare == 0 && strlen(rhs) < adjustedPmnkSize))
            return false; // don't check tail
        else // the pmnk are identical, need to look at tails
        {
            bool state = (compareTail(rhs + adjustedPmnkSize) > 0);
            return state;
        }
    }

    constexpr void dropAnchorCopyKeysSortIndex(IndexString indexArr[], std::size_t size) const
    // parameters should look like (indexArray, array count)
    {
        if (charRowAnchors[rowAnchorOffset] == 0) throw Bad_RowString_Anchor(); // no table to index
        if (sizeof(indexArr[0]) != sizeof(h)) throw Item_Size_Mismatch();
        if (charIndexAnchors[indexAnchorOffset] != 0) throw Already_IndexString_Anchor(); // came here twice

        charIndexAnchors[indexAnchorOffset] = (char*)indexArr[0].h.pmnk - offsetof(IndexStringStruct, pmnk);
        indexAnchors[indexAnchorOffset] = &indexArr[0];
        indexCounts[indexAnchorOffset] = size;

        for (std::size_t i = 0; i < size; ++i)
        {
            indexArr[i].copyKey();
        }
        compares = 0;
        swaps = 0;
        quickSortInvoke(indexArr, size);
        cout << "Index (" << indexArr <<") element size = " << sizeof(indexArr[0]) << " bytes, total size = " << sizeof(indexArr[0])*size << " bytes." <<  endl;
    }

    // prints the row pointed to by the IndexStringStruct h.k offset
    friend ostream& operator<< (ostream &os, const IndexString& rhs)
    {
        if (charRowAnchors[rowAnchorOffset] == 0) throw Bad_RowString_Anchor(); // no table to index
        if (charIndexAnchors[indexAnchorOffset] == 0 || rhs.h.k < 0)
            throw Bad_IndexString_Anchor(); // no index for this column
        os << rhs.h.pmnk;
        if (strlen(rhs.h.pmnk) == adjustedPmnkSize) // if the pmnk is full length, there is possibly a tail
        {
            char *rhsRowStr = charRowAnchors[rowAnchorOffset] + (sizeof(((rowType*)0)->r) * rhs.h.k); // points to first byte of rhs row
            rowType *rhsRow = (rowType*)(rhsRowStr); // rhs row pointer
            char *rhsColumnStr = rhsRowStr + rhsRow->r.columnOffset[columnId[indexAnchorOffset]] + adjustedPmnkSize; // points after adjustedPmnkSize bytes
            os << rhsColumnStr;
        }
        return os;
    }

    constexpr void reserve(int i) const { } // no-op
};

#endif // INDEXSTRING_H_INCLUDED


