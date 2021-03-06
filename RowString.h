#ifndef ROWSTRING_H_INCLUDED
#define ROWSTRING_H_INCLUDED

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

RowString.h - header file for RowString class (variation of Direct) in simple arrays for tables

  Both the RowString template class and its accessing IndexString template class share the same 5
  basic template parameters and operate on character <typename T> strings, although the current
  implementation is unabashedly char typed and won’t work for other string types (yet.) Both classes
  are completely base + offset, accessible under virtual addressing, and the objects are portable
  individually as elements or as slabs: for ease of replication. The variable format is the log record
  format. The current demo code base does not use mmap for accessing these objects, but that will work
  just fine in future.

  RowString variables are variable-length (with an upper bound) collections of null-terminated byte
  strings that have their bytes moved as a single body when they are swapped as in sorting, not just
  moving their pointers (following the Sort Benchmark rules.) RowString has "value-string semantics"
  as Stroustrup defined for his String type (The C++11 Programming Language, 4th edition, chapter 19.3).

  Imitating the b-tree block there is an offset, in the front of the RowString object, to the column
  count and column offsets at the end. This allows quick loading from simple .csv files (these
  “comma-separated-value” files are loaded using one of three member functions named assignColumns)
  without commented or escape characters, by copying in each string after the initial offset and
  simply converting the commas to nulls as you build the column count and offsets. That’s very fast
  for loading.

  There are three assignment functions provided for moving char strings into a RowString:
	•	assign (const char* str): which is for a simple one column RowString.
	•	assign (const char* str, std::size_t size): for which the user has preformatted the RowString
		correctly with columns, nulls, counters and offsets.
	•	assignColumns (const char* str, const char delimiter = ','): described in the paragraph above.

  An effort is made to provide as many constexpr member functions as possible, to supply type
  information at compile time for generic programming. That effort pays off during the variadic
  template parameter pack recursion later (QueryPlan and JoinedRow), which is effectively an exercise
  in manufacturing arrays of constants and a little bit of code from typed parameter constants at
  compile time.

  Both RowString and IndexString make extensive use of static global constants and the enum class
  constants Table (identifying all the table arrays of RowString) and Column (identifying all the
  index arrays of IndexString.)

  Accessor Member Functions and Dropping the Anchor

  Two accessor functions were found to be necessary on the RowString class, which are used heavily in
  join operations:

	•	char* columnStr(Column columnEnum): This returns a char pointer into the column inside the row
		element being operated on.
	•	rowType row(): This returns a typed pointer on the row itself.

  After supplying the basic comparison operators for the RowString class (which are there for
  convenience, since tables are not for sorting, that’s what indexes are for) and the input and output
  ostream operators >> and <<, a member function called dropAnchor(RowString rowArr[], std::size_t
  size) is specified, for the purpose of storing the “base” of the base + offset for the table array
  (slab) of RowString elements and its size.

  This anchor allows the RowString element of an array (slab) of RowString elements to know
  information about the array that is not passed or is lost in successive calls or use of constexpr.
  That allows every RowString element to know where it fits in the original table structure, even when
  it is copied out by reference as a (PMNK, K-value) element (where the K-value is the array index of
  the original element) in an IndexString into a pivot element, or temporary variable or array during
  sorting. The anchors, where they are required in these classes, are the source of all base + offset
  referencing by other classes in this relational database system.
*/

#include <iostream>
#include <stddef.h>
#include <algorithm>
#include <string.h>
#include <cstring>

using namespace std;

// My intent is for this to work for more than just typename char someday: wchar_t, char16_t, char32_t
template <typename T, std::size_t maxStringSize, Table tableEnum, std::size_t maxColumnsCount = 4,
         std::size_t maxColumnSize = 1024>
class RowString
{
public:

    static const std::size_t rowLength = maxStringSize;

    struct Body
    {
        T arr[rowLength + 1]; // null termination
    };

    struct Struct
    {
        uint_least16_t columnsCountOffset;
        Body b;
        uint_least16_t columnsCount;
        uint_least16_t columnOffset[maxColumnsCount];
    };

    // Notice, no default assignments above, which yields an empty default constructor, which
    // means release builds (dropping empty functions) will not have quadratic behavior for
    // cross-container element movement (IMHO)

public:
    Struct r;

    typedef RowString<T, maxStringSize, tableEnum, maxColumnsCount> rowType;

    class Assign_String_Too_Long : public runtime_error
    {
    public:
        Assign_String_Too_Long() :
            runtime_error("assign function called with overlong string") {}
    };

    class Item_Size_Mismatch : public runtime_error
    {
    public:
        Item_Size_Mismatch() :
            runtime_error("array[0].structSize() must equal array items/arraySize (possible compiler dependency)") {}
    };

    class Bad_RowString_Anchor : public runtime_error
    {
    public:
        Bad_RowString_Anchor() :
            runtime_error("after allocation, you MUST call rowArray[0].dropAnchor(rowArray, correct_arrayCount)") {}
    };

    class Already_Array_Anchor : public runtime_error
    {
    public:
        Already_Array_Anchor() :
            runtime_error("you must ONLY ONCE call rowArray[0].dropAnchor(rowArray, correct_arrayCount)") {}
    };

    class Input_Stream_Ended : public runtime_error
    {
    public:
        Input_Stream_Ended() :
            runtime_error("input stream ended") {}
    };

    class Bad_String_Delimiter : public runtime_error
    {
    public:
        Bad_String_Delimiter() :
            runtime_error("null delimiters are illegal") {}
    };

    class Input_Stream_Bad : public runtime_error
    {
    public:
        Input_Stream_Bad() :
            runtime_error("input stream bad") {}
    };

    class Input_Stream_Failed : public runtime_error
    {
    public:
        Input_Stream_Failed() :
            runtime_error("input stream failed") {}
    };

    class Wrong_Column_For_This_Table : public runtime_error
    {
    public:
        Wrong_Column_For_This_Table() :
            runtime_error("the column enum is  for a different table") {}
    };

    class Bad_IndexString_Field_Null_Termination : public runtime_error
    {
    public:
        Bad_IndexString_Field_Null_Termination() :
            runtime_error("index fields MUST BE null terminated within the row array") {}
    };

    constexpr std::size_t size() const noexcept
    {
        return rowLength;
    }

    constexpr std::size_t structSize() const noexcept
    {
        return sizeof(r);
    }

    constexpr void checkUnitLength(std::size_t size) const // parameter size should be (array length/array count)
    {
        if (sizeof(r) != size) throw Item_Size_Mismatch();
    }

    constexpr std::size_t count() const
    {
        return rowCounts[(int)tableEnum];
    }

    // for a simple one column rowString
    constexpr RowString& assign (const char* str) const
    {
        r.columnsCountOffset = offsetof(struct Struct,r.columnsCount);
        r.columnsCount = 1; // Only one column.
        r.columnOffset[0] = {offsetof(struct Struct,r.b)}; // only one column offset.

        int len = strlen(str);
        if (len > rowLength) throw Assign_String_Too_Long();
        strcpy(r.b.arr, str);
        r.b.arr[len] = 0;
        return *this;
    }

    // for a DIY rowString with preset nulls, counters and offsets
    constexpr RowString& assign (const char* str, std::size_t size) const
    {
        if (size > sizeof(r)) throw Assign_String_Too_Long();
        memcpy(r, str, size);
        return *this;
    }

    // You delimit the  columns (optional non-null delimiters) of a null terminated string, we fix it up
    inline RowString& assignColumns (const char* str, const char delimiter = ',')
    {
        if (delimiter == 0) throw Bad_String_Delimiter();
        std::size_t len = strlen(str);
        if (len > rowLength) throw Assign_String_Too_Long();
        strcpy(r.b.arr, str);
        r.columnsCountOffset = offsetof(struct Struct,columnsCount);
        r.columnsCount = 1; // Start off with one column
        r.columnOffset[0] = {offsetof(struct Struct,b)};
        for (std::size_t i = 0; r.b.arr[i] != 0 && i < rowLength && r.columnsCount < maxColumnsCount; i++)
        {
            if (r.b.arr[i] == delimiter)
            {
                r.b.arr[i] = 0; // overwrite the delimiter, null terminating the column field
                // create the offset for the next column beyond the null we just wrote
                r.columnOffset[r.columnsCount++] = offsetof(struct Struct,b) + i + 1;
            }
        }
        return *this;
    }

    inline char* columnStr(Column columnEnum) // returns a pointer into the column inside the row
    {
        if (charRowAnchors[(int)tableEnum] == 0) throw Bad_RowString_Anchor(); // no table to index
        if (column2Table(columnEnum) != tableEnum) throw Wrong_Column_For_This_Table();
        char *lhsColumnStr = r.b.arr - offsetof(struct Struct,b) + r.columnOffset[columnId[(int)columnEnum]];

        std::size_t len = strlen(lhsColumnStr);
        if (len > maxColumnSize) throw Bad_IndexString_Field_Null_Termination();
        return lhsColumnStr;
    }

    constexpr rowType row() const
    {
        return *this;
    }

    // All rowString comparisons default to the first null-terminated column
    constexpr bool operator < (const RowString& rhs) const
    {
        return (strcmp(r.b.arr, rhs.r.b.arr) < 0);
    }

    constexpr bool operator <= (const RowString& rhs) const
    {
        return (strcmp(r.b.arr, rhs.r.b.arr) < 1);
    }

    constexpr bool operator == (const RowString& rhs) const
    {
        return (strcmp(r.b.arr, rhs.r.b.arr) == 0);
    }

    constexpr bool operator != (const RowString& rhs) const
    {
        return (strcmp(r.b.arr, rhs.r.b.arr) != 0);
    }

    constexpr bool operator >= (const RowString& rhs) const
    {
        return (strcmp(r.b.arr, rhs.r.b.arr) > -1);
    }

    constexpr bool operator > (const RowString& rhs) const
    {
        return (strcmp(r.b.arr, rhs.r.b.arr) > 0);
    }

    friend istream& operator>> (istream &is, RowString& rhs)
    {
        char input[rowLength + 300]; // comment lines possibly longer than data
        is.getline(input, rowLength + 300);
        rhs.assignColumns(input);
        return is;
    }

    friend ostream& operator<< (ostream &os, const RowString& rhs)
    {
        for (int i = 0; i < rhs.r.columnsCount; i++)
        {
            // C++ doesn't want me to: strPtr = (char*) rhs.r + rhs.r.columnOffset[i];
            const char* strPtr = rhs.r.b.arr - offsetof(Struct, b.arr) + rhs.r.columnOffset[i];
            os << strPtr;
            if (i < rhs.r.columnsCount - 1) os << ",";
        }
        return os;
    }

    constexpr void dropAnchor(RowString rowArr[], std::size_t size) const
    // parameters should look like (rowArray, array count)
    {
        if (charRowAnchors[(int)tableEnum] != 0) throw Already_Array_Anchor();
        if (rowAnchors[(int)tableEnum] != 0) throw Already_Array_Anchor();
        if (sizeof(rowArr[0]) != sizeof(r)) throw Item_Size_Mismatch();
        charRowAnchors[(int)tableEnum] = (char*)rowArr;
        rowAnchors[(int)tableEnum] = &rowArr[0];
        rowCounts[(int)tableEnum] = size;
        cout << "Table (" << rowArr <<") element size = " << sizeof(rowArr[0]) << " bytes, total size = " << sizeof(rowArr[0])*size << " bytes." <<  endl;
    }

    constexpr void reserve(int i) const { } // no-op
};

#endif // ROWSTRING_H_INCLUDED
