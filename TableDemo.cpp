// Building a table with indexes using Kaldanes

// Please build for release to get performance metrics

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

*/

#include <iostream>
#include <fstream>
#include <climits>
#include <iostream>
#include <stddef.h>
#include <algorithm>
#include <cstring>
#include <tuple>

using namespace std;
// Banners from: http://patorjk.com/software/taag/#p=display&f=ANSI%20Shadow&t=Section
// Using ANSI Shadow font

/*
███████╗      ███╗   ███╗        ███████╗
██╔════╝      ████╗ ████║        ██╔════╝
███████╗      ██╔████╔██║        ███████╗
╚════██║      ██║╚██╔╝██║        ╚════██║
███████║tatic ██║ ╚═╝ ██║etadata ███████║ection
╚══════╝      ╚═╝     ╚═╝        ╚══════╝

Goal: What would reside in a database catalog and execute at runtime (compiler, optimizer, etc.)
instead lives in type space and executes at compile time.

Everything in this section (classes, types, static data, const and constexpr)
is needed for generic programming (compile-time)

*/

// Order is critical ...

// Needed to reinterpret_cast later
static const std::size_t maxColumnSizeDefault = 1024;
static const std::size_t pmnkSizeDefault = 7;

enum class Table
{
    airports,
    airlines,
    routes,

    table_Count,
    nilTable = INT_MIN
};
// insert new tables before table_Count, please

static char* charRowAnchors[(int)Table::table_Count] = {0}; // after the first item, the rest get zeroed.
static void* rowAnchors[(int)Table::table_Count] = {0}; // after the first item, the rest get zeroed.
static std::size_t rowCounts[(int)Table::table_Count] = {0}; // ditto, and index length = table length.

enum class Column // must be in the order of Table, above
{
    // 14 airports table columns
    airportId,
    airportName,
    airportCity,
    airportCountry,
    airportIATA,
    airportICAO,
    airportLatitude,
    airportLongitude,
    airportAltitude,
    airportTimezone,
    airportDST,
    airportTz,
    airportType,
    airportSource,

    airportsDivider,

    // 8 airlines table columns
    airlineId,
    airlineName,
    airlineAlias,
    airlineIATA,
    airlineICAO,
    airlineCallsign,
    airlineCountry,
    airlineActive,

    airlinesDivider,

    // 9 routes table columns
    routeAirline,
    routeAirlineId,
    routeSourceAirport,
    routeSourceAirportId,
    routeDestinationAirport,
    routeDestinationAirportId,
    routeCodeshare,
    routeStops,
    routeEquipment,

    routesDivider,

    columnCount, // accessing the array with this enum's static should segfault
    nilColumn = INT_MIN // accessing the array with this enum's static should segfault
};
// insert new columns in order before columnCount,
// please, then adjust the table of IDs below

// Index metadata resides in these (3) arrays
static char* charIndexAnchors[(int)Column::columnCount] = {0}; // after the first item, the rest get zeroed.
static void* indexAnchors[(int)Column::columnCount] = {0}; // after the first item, the rest get zeroed.
static std::size_t indexCounts[(int)Column::columnCount] = {0}; // ditto, and index length = table length.

typedef std::size_t OffsetValue;
static const OffsetValue trammel = UINT_MAX; // to catch the alignment bugs
static const OffsetValue columnId[(int)Column::columnCount + 2] =
// This array translates from the Column enum to the column ID in the specific table.
// THEY NEED TO BE MAINTAINED IN ALIGNMENT!
// "It must be mounted on a tripod!": see "The Wild Bunch" by Peckinpah (director's cut)
// It is effectively the database catalog metadata.
// Use the assertion checker below
// Sorry, this is the only thing I didn't automate easily and reliably! Variadic templates, later?
{
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,trammel,// 14 valid airports column IDs

    0,1,2,3,4,5,6,7,trammel,// 8 valid airlines column IDs

    0,1,2,3,4,5,6,7,8,trammel,// 9 valid routes column IDs

    trammel,trammel // Everything accessing a trammel as an index should segfault
};

class Improperly_Maintained_columnId_Table : public runtime_error
{
public:
    Improperly_Maintained_columnId_Table() :
        runtime_error("the columnId table relationship to Column enums is broken") {}
};

void columnIdChecker()
{
    if (columnId[(int)Column::airportsDivider] != trammel) throw Improperly_Maintained_columnId_Table();
    if (columnId[(int)Column::airlinesDivider] != trammel) throw Improperly_Maintained_columnId_Table();
    if (columnId[(int)Column::routesDivider] != trammel) throw Improperly_Maintained_columnId_Table();
}

class Invalid_Column_Enum_Class_Member : public runtime_error
{
public:
    Invalid_Column_Enum_Class_Member() :
        runtime_error("the Column enum class element is not a valid member") {}
};

// Had the above gone well this will work fine
constexpr Table column2Table(Column columnEnum)
{
    return (columnEnum < Column::airportsDivider) ? Table::airports
           : (columnEnum < Column::airlinesDivider) ? Table::airlines
           : (columnEnum < Column::routesDivider) ? Table::routes
           : Table::nilTable;
}

#include "RowString.h"
#include "IndexString.h"

const std::size_t airportsMaxLen = 184, airportsColumns = 14, airportsCount = 7184;
const char airportsFile[] = "./airports.csv";
typedef RowString<char, airportsMaxLen, Table::airports, airportsColumns, maxColumnSizeDefault> airportRow;

const std::size_t airlinesMaxLen = 113, airlinesColumns = 8, airlinesCount = 6162;
const char airlinesFile[] = "./airlines.csv";
typedef RowString<char, airlinesMaxLen, Table::airlines, airlinesColumns, maxColumnSizeDefault> airlineRow;

const std::size_t routesMaxLen = 65, routesColumns = 9, routesCount = 67663;
const char routesFile[] = "./routes.csv";
typedef RowString<char, routesMaxLen, Table::routes, routesColumns, maxColumnSizeDefault> routeRow;

// Compile time (static) programming is difficult in c++11
template<Column columnEnum>
constexpr decltype((columnEnum < Column::airportsDivider) ? (airportRow*)0
                   : (columnEnum < Column::airlinesDivider) ? (airlineRow*)0
                   : (columnEnum < Column::routesDivider) ? (routeRow*)0
                   : (void*)0) column2RowStringPtr()
{
    return (decltype((columnEnum < Column::airportsDivider) ? (airportRow*)0
                     : (columnEnum < Column::airlinesDivider) ? (airlineRow*)0
                     : (columnEnum < Column::routesDivider) ? (routeRow*)0
                     : (void*)0)) (columnEnum < Column::airportsDivider) ? reinterpret_cast<airportRow*>(rowAnchors[(std::size_t)Table::airports])
           : (columnEnum < Column::airlinesDivider) ? reinterpret_cast<airlineRow*>(rowAnchors[(std::size_t)Table::airlines])
           : (columnEnum < Column::routesDivider) ? reinterpret_cast<routeRow*>(rowAnchors[(std::size_t)Table::routes])
           : (void*)0;
}

typedef IndexString<Column::airportId, char, airportsMaxLen, Table::airports, airportsColumns, maxColumnSizeDefault, pmnkSizeDefault> airportIdType;
typedef IndexString<Column::airportName, char, airportsMaxLen, Table::airports, airportsColumns, maxColumnSizeDefault, pmnkSizeDefault> airportNameType;
typedef IndexString<Column::airportCity, char, airportsMaxLen, Table::airports, airportsColumns, maxColumnSizeDefault, pmnkSizeDefault> airportCityType;
typedef IndexString<Column::airportIATA, char, airportsMaxLen, Table::airports, airportsColumns, maxColumnSizeDefault, pmnkSizeDefault> airportIATAType;
typedef IndexString<Column::airportICAO, char, airportsMaxLen, Table::airports, airportsColumns, maxColumnSizeDefault, pmnkSizeDefault> airportICAOType;
typedef IndexString<Column::airlineId, char, airlinesMaxLen, Table::airlines, airlinesColumns, maxColumnSizeDefault, pmnkSizeDefault> airlineIdType;
typedef IndexString<Column::routeAirlineId, char, routesMaxLen, Table::routes, routesColumns, maxColumnSizeDefault, pmnkSizeDefault> routeAirlineIdType;
typedef IndexString<Column::routeSourceAirportId, char, routesMaxLen, Table::routes, routesColumns, maxColumnSizeDefault, pmnkSizeDefault> routeSourceAirportIdType;
typedef IndexString<Column::routeDestinationAirportId, char, routesMaxLen, Table::routes, routesColumns, maxColumnSizeDefault, pmnkSizeDefault> routeDestinationAirportIdType;

#include "RelationVector.h"

typedef RelationVector<airportIdType, Column::airportId, routeSourceAirportIdType, Column::routeSourceAirportId> airportId2RouteSourceAirportIdType;
typedef RelationVector<routeDestinationAirportIdType, Column::routeDestinationAirportId, airportIdType, Column::airportId> routeDestinationAirportId2AirportIdType;
typedef RelationVector<routeAirlineIdType, Column::routeAirlineId, airlineIdType, Column::airlineId> routeAirlineId2AirlineIdType;

typedef tuple<airportId2RouteSourceAirportIdType, routeDestinationAirportId2AirportIdType, routeAirlineId2AirlineIdType> routesJoinRelationsTupleType;

// could binary search on c_str from join row offset index -> constexpr auto (c_str) -> returns decltype of index (tuple<n> or variadic)

#include "JoinedRow.h"
typedef QueryPlan<airportId2RouteSourceAirportIdType, routeDestinationAirportId2AirportIdType, routeAirlineId2AirlineIdType> routesQueryPlanType;
typedef JoinedRow<routesJoinRelationsTupleType, trammel, airportId2RouteSourceAirportIdType, routeDestinationAirportId2AirportIdType, routeAirlineId2AirlineIdType> routesJoinedRowType;

/*
██████╗        ██████╗   ███████╗
██╔══██╗       ██╔══██╗  ██╔════╝
██████╔╝       ██████╔╝  ███████╗
██╔══██╗       ██╔═══╝   ╚════██║
██║  ██║untime ██║rogram ███████║ection
╚═╝  ╚═╝       ╚═╝       ╚══════╝

Nothing in this section has definitions that the generic programming (compile-time) requires.

*/

class Boolian_Error : public runtime_error
{
public:
    Boolian_Error() :
        runtime_error("boolian operator failure") {}
};

class Could_Not_Open_File : public runtime_error
{
public:
    Could_Not_Open_File() :
        runtime_error("file name could not be opened") {}
};

template<typename T>
void loadCSVFile(T arr[], int size, const char filename[])
{
    ifstream csvFile(filename);
    if (!csvFile)
    {
        throw Could_Not_Open_File();
    }
    for (int i = 0; (csvFile) && i <= size; i++)
    {
        char input;
        csvFile >> input;
        if (input == '#') // pass over R Language comment lines beginning with a '#'
        {
            csvFile.ignore(numeric_limits<streamsize>::max(), '\n');
            i--;
        }
        else
        {
            csvFile.putback(input);
            csvFile >> arr[i];
        }
    }
    csvFile.close();
}

class Type_Not_Supported_Here : public runtime_error
{
public:
    Type_Not_Supported_Here() :
        runtime_error("that type is not supported in this function") {}
};

template<typename T>
void printTable(T arr[], int size)
{
    typedef typename std::remove_reference<T>::type TR;
    string typeName = typeid(TR).name();
    char isIndexString[] = "IndexString";
    char isRowString[] = "RowString";
    char isJoinedRow[] = "JoinedRow";
    if (strstr(typeName.c_str(), isIndexString))
    {
        // probably should list the friend classes first, in case of detailed demangling
        for (int i = 0; i < size; i++) cout << *(arr[i].row()) << endl;
    }
    else if (strstr(typeName.c_str(), isRowString))
    {
        for (int i = 0; i < size; i++) cout << arr[i] << endl;
    }
    else if (strstr(typeName.c_str(), isJoinedRow))
    {
        for (int i = 0; i < size; i++)
        {
            cout << "(" << arr[i] << ")" << endl;
            if (i < size - 1) cout << "+ ";
        }
    }
    else throw Type_Not_Supported_Here();
}

template<typename T>
void printColumn(T arr[], int size)
{
    typedef typename std::remove_reference<T>::type TR;
    string typeName = typeid(TR).name();
    char isIndexString[] = "IndexString";
    if (strstr(typeName.c_str(), isIndexString))
    {
        // probably should list the friend classes first, in case of detailed demangling
        for (int i = 0; i < size; i++)
        {
            cout << arr[i] << endl;
//            cout << arr[i];
//            if (i + 1 < size) cout << " ";
//            else cout << endl;
        }
    }
    else throw Type_Not_Supported_Here();
}

void asserts()
{
    columnIdChecker();
}

int main()
{
    //airports, airlines, routes
    try
    {
        // Anonyomous catch -> exception names in release code execution

        asserts();

        // Tables
        airportRow airports[airportsCount];
        airports[0].dropAnchor(airports, airportsCount);
        loadCSVFile(airports, airportsCount, airportsFile);

        airlineRow airlines[airlinesCount];
        airlines[0].dropAnchor(airlines, airlinesCount);
        loadCSVFile(airlines, airlinesCount, airlinesFile);

        routeRow routes[routesCount];
        routes[0].dropAnchor(routes, routesCount);
        loadCSVFile(routes, routesCount, routesFile);

        // Index space allocation
        airportIdType airportsId[airportsCount];
        airportNameType airportsName[airportsCount];
        airportCityType airportsCity[airportsCount];
        airportIATAType airportsIATA[airportsCount];
        airportICAOType airportsICAO[airportsCount];
        airlineIdType airlinesId[airlinesCount];
        routeAirlineIdType routesAirlineId[routesCount];
        routeSourceAirportIdType routesSourceAirportId[routesCount];
        routeDestinationAirportIdType routesDestinationAirportId[routesCount];

        // Building the indexes
        airportsId[airportsCount].dropAnchorCopyKeysSortIndex(airportsId, airportsCount);
        airportsName[airportsCount].dropAnchorCopyKeysSortIndex(airportsName, airportsCount);
        airportsCity[airportsCount].dropAnchorCopyKeysSortIndex(airportsCity, airportsCount);
        airportsIATA[airportsCount].dropAnchorCopyKeysSortIndex(airportsIATA, airportsCount);
        airportsICAO[airportsCount].dropAnchorCopyKeysSortIndex(airportsICAO, airportsCount);
        airlinesId[airlinesCount].dropAnchorCopyKeysSortIndex(airlinesId, airlinesCount);
        routesAirlineId[routesCount].dropAnchorCopyKeysSortIndex(routesAirlineId, routesCount);
        routesSourceAirportId[routesCount].dropAnchorCopyKeysSortIndex(routesSourceAirportId, routesCount);
        routesDestinationAirportId[routesCount].dropAnchorCopyKeysSortIndex(routesDestinationAirportId, routesCount);

        cout << "First, let's execute a point query lambda on the airportsName index, for an airport that exists, and one that is imaginary." << endl << endl;

        int rowIndex;
        airportNameType* airportPtr;
        routeAirlineIdType* routesPtr;

        auto printAirports = [&] (string name, std::size_t count = 5)
        {
            rowIndex = binarySearch(airportsName, airportsCount, name.c_str());
            airportPtr = airportsName + abs(rowIndex) - 2;
            cout << "\"" << name << "\" Airport record search:" << endl;
            cout << "Returns: " << rowIndex << ((rowIndex < 0) ? " (Missing)" : " (Existing)") << endl << endl;
            printTable(airportPtr, count);
            cout << endl;
        };

        printAirports("La Guardia Airport");
        printAirports("La Nunca Airport");

        cout << "And, let's execute a point query lambda on the routesAirlineId index, landing in the middle of the Southwest Airlines (WN) routes." << endl << endl;

        auto printRoutes = [&] (string name, std::size_t count = 50)
        {
            rowIndex = binarySearch(routesAirlineId, routesCount, name.c_str());
            routesPtr = routesAirlineId + abs(rowIndex) - 25;
            cout << "\"" << name << "\" Routes record search:" << endl;
            cout << "Returns: " << rowIndex << ((rowIndex < 0) ? " (Missing)" : " (Existing)") << endl << endl;
            printTable(routesPtr, count);
            cout << endl;
        };

        printRoutes("4547");

        cout << "Second, let's make some relation vectors with 'from' and 'to' indexes for each, and see that we can access the table rows from there." << endl << endl;

        airportId2RouteSourceAirportIdType airportId2RouteSourceAirportId;
        cout << "Relation Vector airportId2RouteSourceAirportId.fromIndex[24]: " << endl << ((airportId2RouteSourceAirportId.fromIndex())->row())[23] << endl << endl;
        cout << "Relation Vector airportId2RouteSourceAirportId byte size: " << sizeof(airportId2RouteSourceAirportId) << endl << endl;

        routeDestinationAirportId2AirportIdType routeDestinationAirportId2AirportId;

        routeAirlineId2AirlineIdType routeAirlineId2AirlineId;
        cout << "Relation Vector routeAirlineId2AirlineId.toIndex[15]: " << endl << ((routeAirlineId2AirlineId.toIndex())->row())[15] << endl << endl;

        cout << "Third, let's make a tuple of three relation vectors that will form a join, and see that we can access the same table rows from there." << endl << endl;

        routesJoinRelationsTupleType RoutesJoinRelationsTuple(airportId2RouteSourceAirportId, routeDestinationAirportId2AirportId, routeAirlineId2AirlineId);
        cout << "Routes Join tuple<0> 'from' airportId2RouteSourceAirportId.fromIndex[24] (same as above): " << endl << ((get<0>(RoutesJoinRelationsTuple).fromIndex())->row())[23] << endl << endl;
        cout << "Routes Join tuple<2> 'to' routeAirlineId2AirlineId.toIndex[15] (same as above): " << endl << ((get<2>(RoutesJoinRelationsTuple).toIndex())->row())[15] << endl << endl;
        cout << "Routes Join tuple byte size: " << sizeof(RoutesJoinRelationsTuple) << endl << endl;

        cout << "Fourth, let's make a linked (and checked) QueryPlan of the joined row tables from the relation vectors, and see if we can access the same table rows from there." << endl <<  endl;

        routesQueryPlanType routesQueryPlan(airportId2RouteSourceAirportId, routeDestinationAirportId2AirportId, routeAirlineId2AirlineId);

        routesQueryPlan.printQueryPlanTest();

        //routesJoinedRowType myRoutes[100];
        //cout << endl << myRoutes[0].tableCount() << " table join:" << endl;
        //printTable(myRoutes, 10);

    }
    catch (...)
    {
        std::exception_ptr p = std::current_exception();
        cerr << "Exception: "
             << (p ?
                 p.__cxa_exception_type()->name() :
                 "Anonymous")
             << endl;
    }
    return 0;
}

