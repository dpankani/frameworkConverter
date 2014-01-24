#include <stdio.h>
/*#include "swmmComponents\error.h"
#include "swmmComponents\funcs.h"
#include "swmmComponents\text.h"*/


#include "swmmComponents\consts.h"
#include "swmmComponents\macros.h"
#include "swmmComponents\enums.h"
#include "swmmComponents\error.h"
#include "swmmComponents\datetime.h"
#include "swmmComponents\objects.h"
#include "swmmComponents\funcs.h"
#include "swmmComponents\text.h"
#include "swmmComponents\keywords.h"
#define  EXTERN extern
#include "swmmComponents\globals.h"


#define WRITE(x, y) (report_writeLine((x),y))    //From report.c
#define MAXFNAME        259         // Max. # characters in file name ///// Taken from consts.h line 25
#define MAXTOKS			40
#define MAXLINE         1024        // Max. # characters per input line
#define TRUE			1           // Value for TRUE state // from consts.h
#define MAX_NODE_TYPES 4			// Max number of SWMM node types
//---------------------------
// Token separator characters
//--------------------------- 
#define   SEPSTR    " \t\n\r"

/*//from Mathexpr.h
//  Node in a tokenized math expression list
struct ExprNode
{
    int    opcode;                // operator code
    int    ivar;                  // variable index
    double fvalue;                // numerical value
	struct ExprNode *prev;        // previous node
    struct ExprNode *next;        // next node
};
typedef struct ExprNode MathExpr;

//From Object.h
//-----------------
// FILE INFORMATION
//-----------------
typedef struct
{
   char          name[MAXFNAME+1];     // file name
   char          mode;                 // NO_FILE, SCRATCH, USE, or SAVE
   char          state;                // current state (OPENED, CLOSED)
   FILE*         file;                 // FILE structure pointer
}  TFile;


//-----------------------------------------
// LINKED LIST ENTRY FOR TABLES/TIME SERIES
//-----------------------------------------
struct  TableEntry
{
   double  x;
   double  y;
   struct  TableEntry* next;
};
typedef struct TableEntry TTableEntry;

//-------------------------
// CURVE/TIME SERIES OBJECT
//-------------------------
typedef struct
{
   char*         ID;              // Table/time series ID
   int           curveType;       // type of curve tabulated
   int           refersTo;        // reference to some other object            //(5.0.010 - LR)
   double        dxMin;           // smallest x-value interval                 //(5.0.014 - LR)
   double        lastDate;        // last input date for time series
   double        x1, x2;          // current bracket on x-values
   double        y1, y2;          // current bracket on y-values
   TTableEntry*  firstEntry;      // first data point
   TTableEntry*  lastEntry;       // last data point
   TTableEntry*  thisEntry;       // current data point
   TFile         file;            // external data file                        //(5.0.014 - LR)
}  TTable;
//------------------------------
// DIRECT EXTERNAL INFLOW OBJECT
//------------------------------
struct ExtInflow
{
   int            param;         // pollutant index (flow = -1)
   int            type;          // CONCEN or MASS
   int            tSeries;       // index of inflow time series
   int            basePat;       // baseline time pattern                      //(5.0.014 - LR)
   double         cFactor;       // units conversion factor for mass inflow
   double         baseline;      // constant baseline value
   double         sFactor;       // time series scaling factor
   struct ExtInflow* next;       // pointer to next inflow data object
};
typedef struct ExtInflow TExtInflow;


//-------------------------------
// DRY WEATHER FLOW INFLOW OBJECT
//-------------------------------
struct DwfInflow
{
   int            param;          // pollutant index (flow = -1)
   double         avgValue;       // average value (cfs or concen.)
   int            patterns[4];    // monthly, daily, hourly, weekend time patterns
   struct DwfInflow* next;        // pointer to next inflow data object
};
typedef struct DwfInflow TDwfInflow;


//-------------------
// RDII INFLOW OBJECT
//-------------------
typedef struct
{
   int           unitHyd;         // index of unit hydrograph
   double        area;            // area of sewershed (ft2)
}  TRdiiInflow;

//-----------------
// TREATMENT OBJECT
//-----------------
typedef struct
{
    int          treatType;       // treatment equation type: REMOVAL/CONCEN
    MathExpr    *equation;        // treatment eqn. as tokenized math terms    //(5.0.010 - LR)
} TTreatment;

//-----------------
// POLLUTANT OBJECT
//-----------------
typedef struct
{
   char*         ID;              // Pollutant ID
   int           units;           // units
   double        mcf;             // mass conversion factor
   double        dwfConcen;       // dry weather sanitary flow concen.         //(5.0.017 - LR)
   double        pptConcen;       // precip. concen.
   double        gwConcen;        // groundwater concen.
   double        rdiiConcen;      // RDII concen.
   double        kDecay;          // decay constant (1/sec)
   int           coPollut;        // co-pollutant index
   double        coFraction;      // co-pollutant fraction
   int           snowOnly;        // TRUE if buildup occurs only under snow
}  TPollut;

//------------
// NODE OBJECT
//------------
typedef struct
{
   char*         ID;              // node ID
   int           type;            // node type code
   int           subIndex;        // index of node's sub-category
   char          rptFlag;         // reporting flag
   double        invertElev;      // invert elevation (ft)
   double        initDepth;       // initial storage level (ft)
   double        fullDepth;       // dist. from invert to surface (ft)
   double        surDepth;        // added depth under surcharge (ft)
   double        pondedArea;      // area filled by ponded water (ft2)
   TExtInflow*   extInflow;       // pointer to external inflow data
   TDwfInflow*   dwfInflow;       // pointer to dry weather flow inflow data
   TRdiiInflow*  rdiiInflow;      // pointer to RDII inflow data
   TTreatment*   treatment;       // array of treatment data

   int           degree;          // number of outflow links
   char          updated;         // true if state has been updated
   double        crownElev;       // top of highest connecting conduit (ft)
   double        inflow;          // total inflow (cfs)
   double        outflow;         // total outflow (cfs)
   double        oldVolume;       // previous volume (ft3)
   double        newVolume;       // current volume (ft3)
   double        fullVolume;      // max. storage available (ft3)
   double        overflow;        // overflow rate (cfs)
   double        oldDepth;        // previous water depth (ft)
   double        newDepth;        // current water depth (ft)
   double        oldLatFlow;      // previous lateral inflow (cfs)
   double        newLatFlow;      // current lateral inflow (cfs)
   double*       oldQual;         // previous quality state
   double*       newQual;         // current quality state

   //double*       wStored;                                                    //(5.0.018 - LR)

   double        oldFlowInflow;   // previous flow inflow
   double        oldNetInflow;    // previous net inflow
}  TNode;


//--------------------
// SUBCATCHMENT OBJECT
//--------------------
typedef struct
{
   char*         ID;              // subcatchment name
   char          rptFlag;         // reporting flag
   int           gage;            // raingage index
   int           outNode;         // outlet node index
   int           outSubcatch;     // outlet subcatchment index
   int           infil;           // infiltration object index
   //TSubarea      subArea[3];      // sub-area data
   double        width;           // overland flow width (ft)
   double        area;            // area (ft2)
   double        fracImperv;      // fraction impervious
   double        slope;           // slope (ft/ft)
   double        curbLength;      // total curb length (ft)
   double*       initBuildup;     // initial pollutant buildup (mass/ft2)
   //TLandFactor*  landFactor;      // array of land use factors
   //TGroundwater* groundwater;     // associated groundwater data
   //TSnowpack*    snowpack;        // associated snow pack data

   double        lidArea;         // area devoted to LIDs (ft2)                //(5.0.019 - LR)
   double        rainfall;        // current rainfall (ft/sec)
   double        losses;          // current infil + evap losses (ft/sec)
   double        runon;           // runon from other subcatchments (cfs)
   double        oldRunoff;       // previous runoff (cfs)
   double        newRunoff;       // current runoff (cfs)
   double        oldSnowDepth;    // previous snow depth (ft)
   double        newSnowDepth;    // current snow depth (ft)
   double*       oldQual;         // previous runoff quality (mass/L)
   double*       newQual;         // current runoff quality (mass/L)
   double*       pondedQual;      // ponded surface water quality (mass/ft3)
   double*       totalLoad;       // total washoff load (lbs or kg)
}  TSubcatch;*/

/*//From enums.h
//-------------------------------------
// Computed node quantities
//-------------------------------------
 #define MAX_NODE_RESULTS 7 
#define MAX_SYS_RESULTS 14
#define MAX_SUBCATCH_RESULTS 7
#define MAX_LINK_RESULTS 6

 enum NodeResultType {
      NODE_DEPTH,                      // water depth above invert
      NODE_HEAD,                       // hydraulic head
      NODE_VOLUME,                     // volume stored & ponded
      NODE_LATFLOW,                    // lateral inflow rate
      NODE_INFLOW,                     // total inflow rate
      NODE_OVERFLOW,                   // overflow rate
      NODE_QUAL};                      // concentration of each pollutant

 enum ObjectType {
      GAGE,                            // rain gage
      SUBCATCH,                        // subcatchment
      NODE,                            // conveyance system node
      LINK,                            // conveyance system link
      POLLUT,                          // pollutant
      LANDUSE,                         // land use category
      TIMEPATTERN,                     // dry weather flow time pattern
      CURVE,                           // generic table of values
      TSERIES,                         // generic time series of values
      CONTROL,                         // conveyance system control rules
      TRANSECT,                        // irregular channel cross-section
      AQUIFER,                         // groundwater aquifer
      UNITHYD,                         // RDII unit hydrograph
      SNOWMELT,                        // snowmelt parameter set
      SHAPE,                           // custom conduit shape                 //(5.0.010 - LR)
      LID,                             // LID treatment units                  //(5.0.019 - LR)
      MAX_OBJ_TYPES}; 
//-------------------------------------
// Names of Node sub-types
//-------------------------------------
 #define MAX_NODE_TYPES 4
 enum NodeType {
      JUNCTION,
      OUTFALL,
      STORAGE,
      DIVIDER};

//-------------------------------------
// Names of Link sub-types
//-------------------------------------
 #define MAX_LINK_TYPES 5
 enum LinkType {
      CONDUIT,
      PUMP,
      ORIFICE,
      WEIR,
      OUTLET};

 enum InputSectionType {
      s_TITLE,        s_OPTION,       s_FILE,         s_RAINGAGE,
      s_TEMP,         s_EVAP,         s_SUBCATCH,     s_SUBAREA,
      s_INFIL,        s_AQUIFER,      s_GROUNDWATER,  s_SNOWMELT,
      s_JUNCTION,     s_OUTFALL,      s_STORAGE,      s_DIVIDER,
      s_CONDUIT,      s_PUMP,         s_ORIFICE,      s_WEIR,
      s_OUTLET,       s_XSECTION,     s_TRANSECT,     s_LOSSES,
      s_CONTROL,      s_POLLUTANT,    s_LANDUSE,      s_BUILDUP,
      s_WASHOFF,      s_COVERAGE,     s_INFLOW,       s_DWF,
      s_PATTERN,      s_RDII,         s_UNITHYD,      s_LOADING,
      s_TREATMENT,    s_CURVE,        s_TIMESERIES,   s_REPORT,
      s_COORDINATE,   s_VERTICES,     s_POLYGON,      s_LABEL,
      s_SYMBOL,       s_BACKDROP,     s_TAG,          s_PROFILE,
      s_MAP,          s_LID_CONTROL,  s_LID_USAGE};                            //(5.0.019 - LR)

 enum InputOptionType {
      FLOW_UNITS,        INFIL_MODEL,       ROUTE_MODEL, 
      START_DATE,        START_TIME,        END_DATE,
      END_TIME,          REPORT_START_DATE, REPORT_START_TIME,
      SWEEP_START,       SWEEP_END,         START_DRY_DAYS,
      WET_STEP,          DRY_STEP,          ROUTE_STEP,
      REPORT_STEP,       ALLOW_PONDING,     INERT_DAMPING,
      SLOPE_WEIGHTING,   VARIABLE_STEP,     NORMAL_FLOW_LTD,
      LENGTHENING_STEP,  MIN_SURFAREA,      COMPATIBILITY,
      SKIP_STEADY_STATE, TEMPDIR,           IGNORE_RAINFALL,                   //(5.0.010 - LR)
      FORCE_MAIN_EQN,    LINK_OFFSETS,      MIN_SLOPE,                         //(5.0.014 - LR)
      IGNORE_SNOWMELT,   IGNORE_GWATER,     IGNORE_ROUTING,                    //(5.0.014 - LR)
      IGNORE_QUALITY};                                                         //(5.0.014 - LR)

	  */
//From output.c
// Definition of 4-byte integer, 4-byte real and 8-byte real types             //(5.0.014 - LR)
#define INT4  int
#define REAL4 float
#define REAL8 double


 //---------------------------------------------------
// Minimum depth for reporting non-zero water quality
//---------------------------------------------------
#define   MIN_WQ_DEPTH  0.01     // ft (= 3 mm)
#define   MIN_WQ_FLOW   0.001    // cfs


//-----------------------------------------------------------------------------
//  Shared variables    
//-----------------------------------------------------------------------------
static INT4      IDStartPos;           // starting file position of ID names
static INT4      InputStartPos;        // starting file position of input data
static INT4      OutputStartPos;       // starting file position of output data
static INT4      BytesPerPeriod;       // bytes saved per simulation time period
static INT4      NsubcatchResults;     // number of subcatchment output variables
static INT4      NnodeResults;         // number of node output variables
static INT4      NlinkResults;         // number of link output variables
static INT4      NumSubcatch;          // number of subcatchments reported on  //(5.0.014 - LR)
static INT4      NumNodes;             // number of nodes reported on          //(5.0.014 - LR)
static INT4      NumLinks;             // number of links reported on          //(5.0.014 - LR)
static INT4      NumPolls;             //dp added number of links reported on  //(5.0.014 - LR)

//static REAL4     SysResults[MAX_SYS_RESULTS];    // values of system output vars.




typedef double DateTime;

#define Y_M_D 0
#define M_D_Y 1
#define D_M_Y 2
#define NO_DATE -693594 // 1/1/0001
#define DATE_STR_SIZE 12
#define TIME_STR_SIZE 9

//datetime.c
static int DateFormat = 0;

//-----------------------------------------------------------------------------
//  Constants
//-----------------------------------------------------------------------------
static const char* MonthTxt[] =
    {"JAN", "FEB", "MAR", "APR",
     "MAY", "JUN", "JUL", "AUG",
     "SEP", "OCT", "NOV", "DEC"};
static const int DaysPerMonth[2][12] =      // days per month
    {{31, 28, 31, 30, 31, 30,               // normal years
      31, 31, 30, 31, 30, 31},
     {31, 29, 31, 30, 31, 30,               // leap years
      31, 31, 30, 31, 30, 31}};
static const int DateDelta = 693594;        // days since 01/01/00
static const double SecsPerDay = 86400.;    // seconds per day

//From text.h
#define FMT12  "\n    Cannot open input file "
#define FMT13  "\n    Cannot open report file "
#define FMT14  "\n    Cannot open output file "

/*//from globals.h
int Nobjects[MAX_OBJ_TYPES];  // Number of each object type
int Nnodes[MAX_NODE_TYPES];   // Number of each node sub-type
int Nlinks[MAX_LINK_TYPES];   // Number of each link sub-type
TTable*    Tseries;                  // Array of time series tables
TFile Finp;                     // Input file
TFile Fout;                     // Output file
DateTime StartDate;                // Starting date
DateTime StartTime;                // Starting time
DateTime StartDateTime;            // Starting Date+Time
DateTime EndDate;                  // Ending date
DateTime EndTime;                  // Ending time
DateTime EndDateTime;              // Ending Date+Time
                  //ReportStartDate,          // Report start date
                 // ReportStartTime,          // Report start time
                  //ReportStart;              // Report start Date+Time*/

//SWMM Driver Data Structures
struct SWMMMetaData {
	int returnresult;
    int fyear;
    int fmonth;
	int fday;
	double fhour;
	int tyear;
	int tmonth;
	int tday;
	double thour;
	int numread;
	double timeStep;
};

struct FrameworkTS {
	char name[MAXLINE];				// time series name
    char description[MAXLINE]; 		// time series description	
	char filePath[MAXFNAME+1]; 
	char frameworkConstituent[MAXLINE];
	char swmmConstituent[MAXLINE];
	char units[MAXLINE];
    int startMonth;
	int startDay;
	int startHour;
	int endMonth;
	int endDay;
	int endHour;
	double toFrameworkConversion;
};

	//Begin definitions for input.c
//int ErrorCode = 0;

//Timeseries export from framework  to SWMM functions
int write_tsblock(FILE* swmmInputFile, FrameworkTS* tsArray, int numTimeSeries, bool hasTSBlock);
int write_inflow_block(int totalNumOfFRWPollutants, char* targetNodeID, char** targetFRWPollutants, char** targetSWMPollutants, double* targetPollutantFactors, FILE* swmmInputFile);
char* get_timeseriesProperties(int propertyType, char* frameworkTSName);

//void report_Nodes(FILE* frpt, FILE* fout);
void report_writeLine(char *line, FILE* frpt);
void output_readDateTime(int period, DateTime* days, int bytesPerPeriod, FILE* fout, int outputStartPos);
void datetime_dateToStr2(DateTime date, char* outResultStr, int format);
void datetime_timeToStr2(DateTime time, char* s);
void output_readNodeResults(int period, int index, FILE* fout);
void datetime_decodeTime(DateTime time, int* h, int* m, int* s);
void datetime_decodeDate(DateTime date, int* year, int* month, int* day);
int isLeapYear(int year);
void divMod(int n, int d, int* result, int* remainder);
FILE* openAnyFile(char *f1, int type);
void  writecon(char *s);
//int output_open(FILE * fout, FILE* ftimeSeries, char** inputs, int*);
SWMMMetaData output_open(FILE* fcontrol, FILE* fout, FILE* ftimeSeries, char** inputs, int* targetPollutantSWMMOrder, int totalNumOfFRWPollutants);
float* output_readNodeResults(float* results, int period, int nodeIndex, int numNodeResults, int numSubCatchs, int numSubCatchRslts, int outputStartPos, int bytesPerPeriod, FILE* fout);
int input_readData2(FILE* finp, char* inputs[20]);
int  getTokens2(char *s, char** outToks);
char* trimwhitespace(char *str);
//int swmm_open(char* f1, char* f2, char* f3);

// Functions for converting a DateTime value to a string
void datetime_dateToStr(DateTime date, char* s);
void datetime_timeToStr(DateTime time, char* s);

// Functions from SWMM5 source
char* sstrncpy(char *dest, const char *src, size_t maxlen);