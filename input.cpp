//-----------------------------------------------------------------------------
//   input.c
//
//   Project:  EPA SWMM5
//   Version:  5.0
//   Date:     6/19/07   (Build 5.0.010)
//             07/30/10  (Build 5.0.019)
//   Author:   L. Rossman
//
//   Input data processing functions.
//-----------------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
  


//#include "headers.h"
//#include "lid.h"                                                               //(5.0.019 - LR)

//-----------------------------------------------------------------------------
//  Constants
//-----------------------------------------------------------------------------
static const int MAXERRS = 100;        // Max. input errors reported

//-----------------------------------------------------------------------------
//  Shared variables
//-----------------------------------------------------------------------------
static char *Tok[MAXTOKS];             // String tokens from line of input
static int  Ntokens;                   // Number of tokens in line of input
static int  Mobjects[MAX_OBJ_TYPES];   // Working number of objects of each type
static int  Mnodes[MAX_NODE_TYPES];    // Working number of node objects
static int  Mlinks[MAX_LINK_TYPES];    // Working number of link objects

//-----------------------------------------------------------------------------
//  External Functions (declared in funcs.h)
//-----------------------------------------------------------------------------
//  input_countObjects  (called by swmm_open in swmm5.c)
//  input_readData      (called by swmm_open in swmm5.c)

//-----------------------------------------------------------------------------
//  Local functions
//-----------------------------------------------------------------------------
static void setDefaults(void);
static int  addObject(int objType, char* id);
static int  getTokens(char *s);
static int  parseLine(int sect, char* line);
static int  readOption(char* line);
static int  readTitle(char* line);
static int  readControl(char* tok[], int ntoks);
static int  readNode(int type);
static int  readLink(int type);

//begin dp imports
char  ErrString[256];
static int  error_setInpError(int errcode, char* s);
void  node_setParams(int j, int type, int k, double x[]);

int  error_setInpError(int errcode, char* s)
{
    strcpy(ErrString, s);
    return errcode;
}
int outfall_readParams(int j, int k, char* tok[], int ntoks)
//
//  Input:   j = node index
//           k = outfall index
//           tok[] = array of string tokens
//           ntoks = number of tokens
//  Output:  returns an error message
//  Purpose: reads an outfall's properties from a tokenized line of input.
//
//  Format of input line is:
//    nodeID  elev  FIXED  fixedStage (flapGate)
//    nodeID  elev  TIDAL  curveID (flapGate)
//    nodeID  elev  TIMESERIES  tseriesID (flapGate)
//    nodeID  elev  FREE (flapGate)
//    nodeID  elev  NORMAL (flapGate)
//
{
    int    i, m, n;
    double x[6];
    char*  id;

    if ( ntoks < 3 ) return error_setInpError(ERR_ITEMS, "");
    id = project_findID(NODE, tok[0]);                      // node ID
    if ( id == NULL )
        return error_setInpError(ERR_NAME, tok[0]);
    if ( ! getDouble(tok[1], &x[0]) )                       // invert elev. 
        return error_setInpError(ERR_NUMBER, tok[1]);
    i = findmatch(tok[2], OutfallTypeWords);               // outfall type
    if ( i < 0 ) return error_setInpError(ERR_KEYWORD, tok[2]);
    x[1] = i;                                              // outfall type
    x[2] = 0.0;                                            // fixed stage
    x[3] = -1.;                                            // tidal curve
    x[4] = -1.;                                            // tide series
    x[5] = 0.;                                             // flap gate
    n = 4;
    if ( i >= FIXED_OUTFALL )
    {
        if ( ntoks < 4 ) return error_setInpError(ERR_ITEMS, "");
        n = 5;
        switch ( i )
        {
        case FIXED_OUTFALL:                                // fixed stage
          if ( ! getDouble(tok[3], &x[2]) )
              return error_setInpError(ERR_NUMBER, tok[3]);
          break;
        case TIDAL_OUTFALL:                                // tidal curve
          m = project_findObject(CURVE, tok[3]);              
          if ( m < 0 ) return error_setInpError(ERR_NAME, tok[3]);
          x[3] = m;
          break;
        case TIMESERIES_OUTFALL:                           // stage time series
          m = project_findObject(TSERIES, tok[3]);            
          if ( m < 0 ) return error_setInpError(ERR_NAME, tok[3]);
          x[4] = m;
          Tseries[m].refersTo = TIMESERIES_OUTFALL;                            //(5.0.019 - LR)
        }
    }
    if ( ntoks == n )
    {
        m = findmatch(tok[n-1], NoYesWords);               // flap gate
        if ( m < 0 ) return error_setInpError(ERR_KEYWORD, tok[n-1]);
        x[5] = m;
    }
    Node[j].ID = id;
    node_setParams(j, OUTFALL, k, x);
    return 0;
}
int storage_readParams(int j, int k, char* tok[], int ntoks)
//
//  Input:   j = node index
//           k = storage unit index
//           tok[] = array of string tokens
//           ntoks = number of tokens
//  Output:  returns an error message
//  Purpose: reads a storage unit's properties from a tokenized line of input.
//
//  Format of input line is:
//     nodeID  elev  maxDepth  initDepth  FUNCTIONAL  a1  a2  a0  aPond  fEvap
//     nodeID  elev  maxDepth  initDepth  TABULAR     curveID  aPond  fEvap
//
{
    int    i, m, n;
    double x[9];
    char*  id;

    // --- get ID name
    if ( ntoks < 6 ) return error_setInpError(ERR_ITEMS, "");
    id = project_findID(NODE, tok[0]);
    if ( id == NULL ) return error_setInpError(ERR_NAME, tok[0]);

    // --- get invert elev, max. depth, & init. depth
    for ( i = 1; i <= 3; i++ )
    {
        if ( ! getDouble(tok[i], &x[i-1]) )
            return error_setInpError(ERR_NUMBER, tok[i]);
    }

    // --- get surf. area relation type
    m = findmatch(tok[4], RelationWords);
    if ( m < 0 ) return error_setInpError(ERR_KEYWORD, tok[4]);
    x[3] = 0.0;                        // a1 
    x[4] = 0.0;                        // a2
    x[5] = 0.0;                        // a0
    x[6] = -1.0;                       // curveID
    x[7] = 0.0;                        // aPond
    x[8] = 0.0;                        // fEvap

    // --- get surf. area function coeffs.
    if ( m == FUNCTIONAL )
    {
        for (i=5; i<=7; i++)
        {
            if ( i < ntoks )
            {
                if ( ! getDouble(tok[i], &x[i-2]) )
                    return error_setInpError(ERR_NUMBER, tok[i]);
            }
        }
        n = 8;
    }

    // --- get surf. area curve name
    else
    {
        m = project_findObject(CURVE, tok[5]);
        if ( m < 0 ) return error_setInpError(ERR_NAME, tok[5]);
        x[6] = m;
        n = 6;
    }

    // --- get ponded area if present 
    if ( ntoks > n)
    {
        if ( ! getDouble(tok[n], &x[7]) )
            return error_setInpError(ERR_NUMBER, tok[n]);
        n++;
    }

    // --- get evaporation fraction if present
    if ( ntoks > n )
    {
        if ( ! getDouble(tok[n], &x[8]) )
            return error_setInpError(ERR_NUMBER, tok[n]);
        n++;                                                                   //(5.0.015 - LR)
    }

    // --- add parameters to storage unit object
    Node[j].ID = id;
    node_setParams(j, STORAGE, k, x);

    // --- read infiltration parameters if present
    //dp if ( ntoks > n ) return storage_readInfilParams(j, tok, ntoks, n);         //(5.0.015 - LR)
    return 0;
}
int divider_readParams(int j, int k, char* tok[], int ntoks)
//
//  Input:   j = node index
//           k = divider index
//           tok[] = array of string tokens
//           ntoks = number of tokens
//  Output:  returns an error message
//  Purpose: reads a flow divider's properties from a tokenized line of input.
//
//  Format of input line is:
//    nodeID  elev  divLink  TABULAR  curveID (optional params)
//    nodeID  elev  divLink  OVERFLOW (optional params)
//    nodeID  elev  divLink  CUTOFF  qCutoff (optional params)
//    nodeID  elev  divLink  WEIR    qMin  dhMax  cWeir (optional params)
//  where optional params are:
//    maxDepth  initDepth  surDepth  aPond    
//
{
    int    i, m, m1, m2, n;
    double x[11];
    char*  id;

    // --- get ID name
    if ( ntoks < 4 ) return error_setInpError(ERR_ITEMS, "");
    id = project_findID(NODE, tok[0]);
    if ( id == NULL ) return error_setInpError(ERR_NAME, tok[0]);

    // --- get invert elev.
    if ( ! getDouble(tok[1], &x[0]) ) return error_setInpError(ERR_NUMBER, tok[1]);

    // --- initialize parameter values
    for ( i=1; i<11; i++) x[i] = 0.0;

    // --- check if no diverted link supplied
    if ( strlen(tok[2]) == 0 || strcmp(tok[2], "*") == 0 ) x[1] = -1.0;

    // --- otherwise get index of diverted link
    else
    {
        m1 = project_findObject(LINK, tok[2]);
        if ( m1 < 0 ) return error_setInpError(ERR_NAME, tok[2]);
        x[1] = m1;
    }
    
    // --- get divider type
	n = 4;
    m1 = findmatch(tok[3], DividerTypeWords);
    if ( m1 < 0 ) return error_setInpError(ERR_KEYWORD, tok[3]);
    x[2] = m1;

    // --- get index of flow diversion curve for Tabular divider
    x[3] = -1;
    if ( m1 == TABULAR_DIVIDER )
    {
        if ( ntoks < 5 ) return error_setInpError(ERR_ITEMS, "");
        m2 = project_findObject(CURVE, tok[4]);
        if ( m2 < 0 ) return error_setInpError(ERR_NAME, tok[4]);
        x[3] = m2;
        n = 5;
    }

    // --- get cutoff flow for Cutoff divider
    if ( m1 == CUTOFF_DIVIDER )
    {
        if ( ntoks < 5 ) return error_setInpError(ERR_ITEMS, "");
        if ( ! getDouble(tok[4], &x[4]) )
            return error_setInpError(ERR_NUMBER, tok[4]);
        n = 5;
    }

    // --- get qmin, dhMax, & cWeir for Weir divider
    if ( m1 == WEIR_DIVIDER )
    {
        if ( ntoks < 7 ) return error_setInpError(ERR_ITEMS, "");
        for (i=4; i<7; i++)
             if ( ! getDouble(tok[i], &x[i]) )
                 return error_setInpError(ERR_NUMBER, tok[i]);
        n = 7;
    }

    // --- no parameters needed for Overflow divider
    if ( m1 == OVERFLOW_DIVIDER ) n = 4;

    // --- retrieve optional full depth, init. depth, surcharged depth
    //      & ponded area
    m = 7;
    for (i=n; i<ntoks && m<11; i++)
    {
        if ( ! getDouble(tok[i], &x[m]) )
        {
            return error_setInpError(ERR_NUMBER, tok[i]);
        }
        m++;
    }
 
    // --- add parameters to data base
    Node[j].ID = id;
    node_setParams(j, DIVIDER, k, x);
    return 0;
}

//=============================================================================
void  node_setParams(int j, int type, int k, double x[])
//
//  Input:   j = node index
//           type = node type code
//           k = index of node type
//           x[] = array of property values
//  Output:  none
//  Purpose: assigns property values to a node.
//
{
    Node[j].type       = type;
    Node[j].subIndex   = k;
    Node[j].invertElev = x[0] / UCF(LENGTH);
    Node[j].crownElev  = Node[j].invertElev;
    Node[j].initDepth  = 0.0;
    Node[j].newVolume  = 0.0;
    Node[j].fullVolume = 0.0;
    Node[j].fullDepth  = 0.0;
    Node[j].surDepth   = 0.0;
    Node[j].pondedArea = 0.0;
    Node[j].degree     = 0;
    switch (type)
    {
      case JUNCTION:
        Node[j].fullDepth = x[1] / UCF(LENGTH);
        Node[j].initDepth = x[2] / UCF(LENGTH);
        Node[j].surDepth  = x[3] / UCF(LENGTH);
        Node[j].pondedArea = x[4] / (UCF(LENGTH)*UCF(LENGTH));
        break;

      case OUTFALL:
        Outfall[k].type        = (int)x[1];
        Outfall[k].fixedStage  = x[2] / UCF(LENGTH);
        Outfall[k].tideCurve   = (int)x[3];
        Outfall[k].stageSeries = (int)x[4];
        Outfall[k].hasFlapGate = (char)x[5];
        break;

      case STORAGE:
        Node[j].fullDepth  = x[1] / UCF(LENGTH);
        Node[j].initDepth  = x[2] / UCF(LENGTH);
        Storage[k].aCoeff  = x[3];
        Storage[k].aExpon  = x[4];
        Storage[k].aConst  = x[5];
        Storage[k].aCurve  = (int)x[6];
        Node[j].pondedArea = x[7] / (UCF(LENGTH)*UCF(LENGTH));
        Storage[k].fEvap   = x[8];
        break;

      case DIVIDER:
        Divider[k].link      = (int)x[1];
        Divider[k].type      = (int)x[2];
        Divider[k].flowCurve = (int)x[3];
        Divider[k].qMin      = x[4] / UCF(FLOW);
        Divider[k].dhMax     = x[5];
        Divider[k].cWeir     = x[6];
        Node[j].fullDepth    = x[7] / UCF(LENGTH);
        Node[j].initDepth    = x[8] / UCF(LENGTH);
        Node[j].surDepth     = x[9] / UCF(LENGTH);
        Node[j].pondedArea   = x[10] / (UCF(LENGTH)*UCF(LENGTH));
        break;
    }
}
int junc_readParams(int j, int k, char* tok[], int ntoks)
//
//  Input:   j = node index
//           k = junction index
//           tok[] = array of string tokens
//           ntoks = number of tokens
//  Output:  returns an error message
//  Purpose: reads a junction's properties from a tokenized line of input.
//
//  Format of input line is:
//     nodeID  elev  maxDepth  initDepth  surDepth  aPond 
{
    int    i;
    double x[6];
    char*  id;

    if ( ntoks < 2 ) return error_setInpError(ERR_ITEMS, "");
    id = project_findID(NODE, tok[0]);
    if ( id == NULL ) return error_setInpError(ERR_NAME, tok[0]);

    // --- parse invert elev., max. depth, init. depth, surcharged depth,
    //     & ponded area values
    for ( i = 1; i <= 5; i++ )
    {
        x[i-1] = 0.0;
        if ( i < ntoks )
        {
            if ( ! getDouble(tok[i], &x[i-1]) )
                return error_setInpError(ERR_NUMBER, tok[i]);
        }
    }

    // --- check for non-negative values (except for invert elev.)
    for ( i = 1; i <= 4; i++ )
    {
        if ( x[i] < 0.0 ) return error_setInpError(ERR_NUMBER, tok[i+1]);
    }

    // --- add parameters to junction object
    Node[j].ID = id;
    node_setParams(j, JUNCTION, k, x);
    return 0;
}
int node_readParams(int j, int type, int k, char* tok[], int ntoks)
//
//  Input:   j = node index
//           type = node type code
//           k = index of node type
//           tok[] = array of string tokens
//           ntoks = number of tokens
//  Output:  returns an error code
//  Purpose: reads node properties from a tokenized line of input.
//
{
    switch ( type )
    {
      case JUNCTION: return junc_readParams(j, k, tok, ntoks);
      case OUTFALL:  return outfall_readParams(j, k, tok, ntoks);
      case STORAGE:  return storage_readParams(j, k, tok, ntoks);
      case DIVIDER:  return divider_readParams(j, k, tok, ntoks);
      default:       return 0;
    }
}

//=============================================================================

int input_countObjects()
//
//  Input:   none
//  Output:  returns error code
//  Purpose: reads input file to determine number of system objects.
//
{
    char  line[MAXLINE+1];             // line from input data file     
    char  wLine[MAXLINE+1];            // working copy of input line   
    char  *tok;                        // first string token of line          
    int   sect = -1, newsect;          // input data sections          
    int   errcode = 0;                 // error code
    int   errsum = 0;                  // number of errors found                   
    int   i;
    long  lineCount = 0;

    // --- initialize number of objects & set default values
    if ( ErrorCode ) return ErrorCode;
    error_setInpError(0, "");
    for (i = 0; i < MAX_OBJ_TYPES; i++) Nobjects[i] = 0;
    for (i = 0; i < MAX_NODE_TYPES; i++) Nnodes[i] = 0;
    for (i = 0; i < MAX_LINK_TYPES; i++) Nlinks[i] = 0;

    // --- make pass through data file counting number of each object
    while ( fgets(line, MAXLINE, Finp.file) != NULL )
    {
        // --- skip blank lines & those beginning with a comment
        lineCount++;
        strcpy(wLine, line);           // make working copy of line
        tok = strtok(wLine, SEPSTR);   // get first text token on line
        if ( tok == NULL ) continue;
        if ( *tok == ';' ) continue;

        // --- check if line begins with a new section heading
        if ( *tok == '[' )
        {
            // --- look for heading in list of section keywords
            newsect = findmatch(tok, SectWords);
            if ( newsect >= 0 )
            {
                sect = newsect;
                continue;
            }
            else
            {
                sect = -1;
                errcode = ERR_KEYWORD;
            }
        }

        // --- if in OPTIONS section then read the option setting
        //     otherwise add object and its ID name (tok) to project
        if ( sect == s_OPTION ) errcode = readOption(line);
        else if ( sect >= 0 )   errcode = addObject(sect, tok);

        // --- report any error found
        if ( errcode )
        {
           //dp report_writeInputErrorMsg(errcode, sect, line, lineCount);
            errsum++;
            if (errsum >= MAXERRS ) break;
        }
    }

    // --- set global error code if input errors were found
    if ( errsum > 0 ) ErrorCode = ERR_INPUT;
    return ErrorCode;
}

//=============================================================================

int input_readData()
//
//  Input:   none
//  Output:  returns error code
//  Purpose: reads input file to determine input parameters for each object.
//
{
    char  line[MAXLINE+1];        // line from input data file
    char  wLine[MAXLINE+1];       // working copy of input line
    char* comment;                // ptr. to start of comment in input line
    int   sect, newsect;          // data sections
    int   inperr, errsum;         // error code & total error count
    int   lineLength;             // number of characters in input line
    int   i;
    long  lineCount = 0;

    // --- initialize working item count arrays
    //     (final counts in Mobjects, Mnodes & Mlinks should
    //      match those in Nobjects, Nnodes and Nlinks).
    if ( ErrorCode ) return ErrorCode;
    error_setInpError(0, "");
    for (i = 0; i < MAX_OBJ_TYPES; i++)  Mobjects[i] = 0;
    for (i = 0; i < MAX_NODE_TYPES; i++) Mnodes[i] = 0;
    for (i = 0; i < MAX_LINK_TYPES; i++) Mlinks[i] = 0;

    // --- initialize starting date for all time series
    for ( i = 0; i < Nobjects[TSERIES]; i++ )
    {
        Tseries[i].lastDate = StartDate + StartTime;
    }

    // --- read each line from input file
    sect = 0;
    errsum = 0;
    rewind(Finp.file);
    while ( fgets(line, MAXLINE, Finp.file) != NULL )
    {
        // --- make copy of line and scan for tokens
        lineCount++;
        strcpy(wLine, line);
        Ntokens = getTokens(wLine);

        // --- skip blank lines and comments
        if ( Ntokens == 0 ) continue;
        if ( *Tok[0] == ';' ) continue;

        // --- check if max. line length exceeded
        lineLength = strlen(line);
        if ( lineLength >= MAXLINE )
        {
            // --- don't count comment if present
            comment = strchr(line, ';');
            if ( comment ) lineLength = comment - line;    // Pointer math here
            if ( lineLength >= MAXLINE )
            {
                inperr = ERR_LINE_LENGTH;
                //dp report_writeInputErrorMsg(inperr, sect, line, lineCount);
                errsum++;
            }
        }

        // --- check if at start of a new input section
        if (*Tok[0] == '[')
        {
            // --- match token against list of section keywords
            newsect = findmatch(Tok[0], SectWords);
            if (newsect >= 0)
            {
                // --- SPECIAL CASE FOR TRANSECTS
                //     finish processing the last set of transect data
                if ( sect == s_TRANSECT )
                    //dp transect_validate(Nobjects[TRANSECT]-1);

                // --- begin a new input section
                sect = newsect;
                continue;
            }
            else
            {
                inperr = error_setInpError(ERR_KEYWORD, Tok[0]);
                //dp report_writeInputErrorMsg(inperr, sect, line, lineCount);
                errsum++;
                break;
            }
        }

        // --- otherwise parse tokens from input line
        else
        {
            inperr = parseLine(sect, line);
            if ( inperr > 0 )
            {
                errsum++;
               //dp if ( errsum > MAXERRS ) report_writeLine(FMT19);
               //dp  else report_writeInputErrorMsg(inperr, sect, line, lineCount);
            }
        }

        // --- stop if reach end of file or max. error count
        if (errsum > MAXERRS) break;
    }   /* End of while */

    // --- check for errors
    if (errsum > 0)  ErrorCode = ERR_INPUT;
    return ErrorCode;
}

//=============================================================================

int  addObject(int objType, char* id)
//
//  Input:   objType = object type index
//           id = object's ID string
//  Output:  returns an error code
//  Purpose: adds a new object to the project.
//
{
    int errcode = 0;
   /* switch( objType )
    {
      case s_RAINGAGE:
        if ( !project_addObject(GAGE, id, Nobjects[GAGE]) )
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[GAGE]++;
        break;

      case s_SUBCATCH:
        if ( !project_addObject(SUBCATCH, id, Nobjects[SUBCATCH]) )
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[SUBCATCH]++;
        break;

      case s_AQUIFER:
        if ( !project_addObject(AQUIFER, id, Nobjects[AQUIFER]) )
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[AQUIFER]++;
        break;

      case s_UNITHYD:
        // --- the same Unit Hydrograph can span several lines
        if ( project_findObject(UNITHYD, id) < 0 )
        {
            if ( !project_addObject(UNITHYD, id, Nobjects[UNITHYD]) )
                errcode = error_setInpError(ERR_DUP_NAME, id);
            Nobjects[UNITHYD]++;
        }
        break;

      case s_SNOWMELT:
        // --- the same Snowmelt object can appear on several lines
        if ( project_findObject(SNOWMELT, id) < 0 )
        {
            if ( !project_addObject(SNOWMELT, id, Nobjects[SNOWMELT]) )
                errcode = error_setInpError(ERR_DUP_NAME, id);
            Nobjects[SNOWMELT]++;
        }
        break;

      case s_JUNCTION:
        if ( !project_addObject(NODE, id, Nobjects[NODE]) )
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[NODE]++;
        Nnodes[JUNCTION]++;
        break;

      case s_OUTFALL:
        if ( !project_addObject(NODE, id, Nobjects[NODE]) )
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[NODE]++;
        Nnodes[OUTFALL]++;
        break;

      case s_STORAGE:
        if ( !project_addObject(NODE, id, Nobjects[NODE]) )
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[NODE]++;
        Nnodes[STORAGE]++;
        break;

      case s_DIVIDER:
        if ( !project_addObject(NODE, id, Nobjects[NODE]) )
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[NODE]++;
        Nnodes[DIVIDER]++;
        break;

      case s_CONDUIT:
        if ( !project_addObject(LINK, id, Nobjects[LINK]) )
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[LINK]++;
        Nlinks[CONDUIT]++;
        break;

      case s_PUMP:
        if ( !project_addObject(LINK, id, Nobjects[LINK]) ) 
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[LINK]++;
        Nlinks[PUMP]++;
        break;

      case s_ORIFICE:
        if ( !project_addObject(LINK, id, Nobjects[LINK]) ) 
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[LINK]++;
        Nlinks[ORIFICE]++;
        break;

      case s_WEIR:
        if ( !project_addObject(LINK, id, Nobjects[LINK]) ) 
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[LINK]++;
        Nlinks[WEIR]++;
        break;

      case s_OUTLET:
        if ( !project_addObject(LINK, id, Nobjects[LINK]) )
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[LINK]++;
        Nlinks[OUTLET]++;
        break;

      case s_POLLUTANT:
        if ( !project_addObject(POLLUT, id, Nobjects[POLLUT]) ) 
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[POLLUT]++;
        break;

      case s_LANDUSE:
        if ( !project_addObject(LANDUSE, id, Nobjects[LANDUSE]) ) 
            errcode = error_setInpError(ERR_DUP_NAME, id);
        Nobjects[LANDUSE]++;
        break;

      case s_PATTERN:
        // --- a time pattern can span several lines
        if ( project_findObject(TIMEPATTERN, id) < 0 )
        {
            if ( !project_addObject(TIMEPATTERN, id, Nobjects[TIMEPATTERN]) )
                errcode = error_setInpError(ERR_DUP_NAME, id);
            Nobjects[TIMEPATTERN]++;
        }
        break;

      case s_CURVE:
        // --- a Curve can span several lines
        if ( project_findObject(CURVE, id) < 0 )
        {
            if ( !project_addObject(CURVE, id, Nobjects[CURVE]) )
                errcode = error_setInpError(ERR_DUP_NAME, id);
            Nobjects[CURVE]++;

            // --- check for a conduit shape curve                             //(5.0.010 - LR)
            id = strtok(NULL, SEPSTR);                                         //(5.0.010 - LR)
            if ( findmatch(id, CurveTypeWords) == SHAPE_CURVE )                //(5.0.010 - LR)
                Nobjects[SHAPE]++;                                             //(5.0.010 - LR)
        }
        break;

      case s_TIMESERIES:
        // --- a Time Series can span several lines
        if ( project_findObject(TSERIES, id) < 0 )
        {
            if ( !project_addObject(TSERIES, id, Nobjects[TSERIES]) )
                errcode = error_setInpError(ERR_DUP_NAME, id);
            Nobjects[TSERIES]++;
        }
        break;

      case s_CONTROL:
        if ( match(id, w_RULE) ) Nobjects[CONTROL]++;
        break;

      case s_TRANSECT:
        // --- for TRANSECTS, ID name appears as second entry on X1 line
        if ( match(id, "X1") )
        {
            id = strtok(NULL, SEPSTR);
            if ( id ) 
            {
                if ( !project_addObject(TRANSECT, id, Nobjects[TRANSECT]) )
                    errcode = error_setInpError(ERR_DUP_NAME, id);
                Nobjects[TRANSECT]++;
            }
        }
        break;

////  Following segment added for SWMM5-LID edition  ////                      //(5.0.019 - LR)
      case s_LID_CONTROL:
        // --- an LID object can span several lines
        if ( project_findObject(LID, id) < 0 )
        {
            if ( !project_addObject(LID, id, Nobjects[LID]) )
                errcode = error_setInpError(ERR_DUP_NAME, id);
            Nobjects[LID]++;
        }
        break;
    }*/
    return errcode;
}

//=============================================================================

int  parseLine(int sect, char *line)
//
//  Input:   sect  = current section of input file
//           *line = line of text read from input file
//  Output:  returns error code or 0 if no error found
//  Purpose: parses contents of a tokenized line of text read from input file.
//
{
    int j, err;
    switch (sect)
    {
      /*case s_TITLE:
        return readTitle(line);

      case s_RAINGAGE:
        j = Mobjects[GAGE];
        err = gage_readParams(j, Tok, Ntokens);
        Mobjects[GAGE]++;
        return err;

      case s_TEMP:
        return climate_readParams(Tok, Ntokens);

      case s_EVAP:
        return climate_readEvapParams(Tok, Ntokens);

      case s_SUBCATCH:
        j = Mobjects[SUBCATCH];
        err = subcatch_readParams(j, Tok, Ntokens);
        Mobjects[SUBCATCH]++;
        return err;

      case s_SUBAREA:
        return subcatch_readSubareaParams(Tok, Ntokens);

      case s_INFIL:
        return infil_readParams(InfilModel, Tok, Ntokens);

      case s_AQUIFER:
        j = Mobjects[AQUIFER];
        err = gwater_readAquiferParams(j, Tok, Ntokens);
        Mobjects[AQUIFER]++;
        return err;

      case s_GROUNDWATER:
        return gwater_readGroundwaterParams(Tok, Ntokens);

      case s_SNOWMELT:
        return snow_readMeltParams(Tok, Ntokens);*/

      case s_JUNCTION:
        return readNode(JUNCTION);

      case s_OUTFALL:
        return readNode(OUTFALL);

      case s_STORAGE:
        return readNode(STORAGE);

      case s_DIVIDER:
        return readNode(DIVIDER);

     /* case s_CONDUIT:
        return readLink(CONDUIT);

      case s_PUMP:
        return readLink(PUMP);

      case s_ORIFICE:
        return readLink(ORIFICE);

      case s_WEIR:
        return readLink(WEIR);

      case s_OUTLET:
        return readLink(OUTLET);

      case s_XSECTION:
        return link_readXsectParams(Tok, Ntokens);

      case s_TRANSECT:
        return transect_readParams(&Mobjects[TRANSECT], Tok, Ntokens);

      case s_LOSSES:
        return link_readLossParams(Tok, Ntokens);

      case s_POLLUTANT:
        j = Mobjects[POLLUT];
        err = landuse_readPollutParams(j, Tok, Ntokens);
        Mobjects[POLLUT]++;
        return err;

      case s_LANDUSE:
        j = Mobjects[LANDUSE];
        err = landuse_readParams(j, Tok, Ntokens);
        Mobjects[LANDUSE]++;
        return err;

      case s_BUILDUP:
        return landuse_readBuildupParams(Tok, Ntokens);

      case s_WASHOFF:
        return landuse_readWashoffParams(Tok, Ntokens);

      case s_COVERAGE:
        return subcatch_readLanduseParams(Tok, Ntokens);

      case s_INFLOW:
        return inflow_readExtInflow(Tok, Ntokens);

      case s_DWF:
        return inflow_readDwfInflow(Tok, Ntokens);

      case s_PATTERN:
        return inflow_readDwfPattern(Tok, Ntokens);

      case s_RDII:
        return rdii_readRdiiInflow(Tok, Ntokens);

      case s_UNITHYD:
        return rdii_readUnitHydParams(Tok, Ntokens);

      case s_LOADING:
        return subcatch_readInitBuildup(Tok, Ntokens);

      case s_TREATMENT:
        return treatmnt_readExpression(Tok, Ntokens);

      case s_CURVE:
        return table_readCurve(Tok, Ntokens);

      case s_TIMESERIES:
        return table_readTimeseries(Tok, Ntokens);

      case s_CONTROL:
        return readControl(Tok, Ntokens);

      case s_REPORT:
        return report_readOptions(Tok, Ntokens);

      case s_FILE:
        return iface_readFileParams(Tok, Ntokens);

      case s_LID_CONTROL:                                                      //(5.0.019 - LR)
        return lid_readProcParams(Tok, Ntokens);                               //(5.0.019 - LR)

      case s_LID_USAGE:
        return lid_readGroupParams(Tok, Ntokens);*/

      default: return 0;
    }
}

//=============================================================================

int readControl(char* tok[], int ntoks)
//
//  Input:   tok[] = array of string tokens
//           ntoks = number of tokens
//  Output:  returns error code
//  Purpose: reads a line of input for a control rule.
//
{
   /* int index;
    int keyword;

    // --- check for minimum number of tokens
    if ( ntoks < 2 ) return error_setInpError(ERR_ITEMS, "");

    // --- get index of control rule keyword
    keyword = findmatch(tok[0], RuleKeyWords);
    if ( keyword < 0 ) return error_setInpError(ERR_KEYWORD, tok[0]);

    // --- if line begins a new control rule, add rule ID to the database
    if ( keyword == 0 )
    {
        if ( !project_addObject(CONTROL, tok[1], Mobjects[CONTROL]) )
            return error_setInpError(ERR_DUP_NAME, Tok[1]);
        Mobjects[CONTROL]++;
    }

    // --- get index of last control rule processed
    index = Mobjects[CONTROL] - 1;
    if ( index < 0 ) return error_setInpError(ERR_RULE, "");

    // --- add current line as a new clause to the control rule
    return controls_addRuleClause(index, keyword, Tok, Ntokens);*/
	return 0;
}

//=============================================================================

int readOption(char* line)
//
//  Input:   line = line of input data
//  Output:  returns error code
//  Purpose: reads an input line containing a project option.
//
{
    /*Ntokens = getTokens(line);
    if ( Ntokens < 2 ) return 0;
    return project_readOption(Tok[0], Tok[1]);
	*/
	return 0;
}

//=============================================================================

int readTitle(char* line)
//
//  Input:   line = line from input file
//  Output:  returns error code
//  Purpose: reads project title from line of input.
//
{
    int i, n;
    for (i = 0; i < MAXTITLE; i++)
    {
        // --- find next empty Title entry
        if ( strlen(Title[i]) == 0 )
        {
            // --- strip line feed character from input line
            n = strlen(line);
            if (line[n-1] == 10) line[n-1] = ' ';

            // --- copy input line into Title entry
            sstrncpy(Title[i], line, MAXMSG);
            break;
        }
    }
    return 0;
}

//=============================================================================
    
int readNode(int type)
//
//  Input:   type = type of node
//  Output:  returns error code
//  Purpose: reads data for a node from a line of input.
//
{
    int j = Mobjects[NODE];
    int k = Mnodes[type];
    int err = node_readParams(j, type, k, Tok, Ntokens);
    Mobjects[NODE]++;
    Mnodes[type]++;
    return err;
}

//=============================================================================

int readLink(int type)
//
//  Input:   type = type of link
//  Output:  returns error code
//  Purpose: reads data for a link from a line of input.
//
{
    int j = Mobjects[LINK];
    int k = Mlinks[type];
    int err = link_readParams(j, type, k, Tok, Ntokens);
    Mobjects[LINK]++;
    Mlinks[type]++;
    return err;
}

//=============================================================================

int  findmatch(char *s, char *keyword[])
//
//  Input:   s = character string
//           keyword = array of keyword strings
//  Output:  returns index of matching keyword or -1 if no match found  
//  Purpose: finds match between string and array of keyword strings.
//
{
   int i = 0;
   while (keyword[i] != NULL)
   {
      if (match(s, keyword[i])) return(i);
      i++;
   }
   return(-1);
}

//=============================================================================

int  match(char *str, char *substr)
//
//  Input:   str = character string being searched
//           substr = sub-string being searched for
//  Output:  returns 1 if sub-string found, 0 if not
//  Purpose: sees if a sub-string of characters appears in a string
//           (not case sensitive).
//
{
    int i,j;

    // --- fail if substring is empty
    if (!substr[0]) return(0);

    // --- skip leading blanks of str
    for (i=0; str[i]; i++)
        if (str[i] != ' ') break;

    // --- check if substr matches remainder of str
    for (i=i,j=0; substr[j]; i++,j++)
        if (!str[i] || UCHAR(str[i]) != UCHAR(substr[j]))
            return(0);
    return(1);
}

//=============================================================================

int  getInt(char *s, int *y)
//
//  Input:   s = a character string
//  Output:  y = converted value of s,
//           returns 1 if conversion successful, 0 if not
//  Purpose: converts a string to an integer number.
//
{
    double x;
    if ( getDouble(s, &x) )
    {
        if ( x < 0.0 ) x -= 0.01;
        else x += 0.01;
        *y = (int)x;
        return 1;
    }
    *y = 0;
    return 0;
}

//=============================================================================

int  getFloat(char *s, float *y)
//
//  Input:   s = a character string
//  Output:  y = converted value of s,
//           returns 1 if conversion successful, 0 if not
//  Purpose: converts a string to a single precision floating point number.
//
{
    char *endptr;
    *y = (float) strtod(s, &endptr);
    if (*endptr > 0) return(0);
    return(1);
}

//=============================================================================

int  getDouble(char *s, double *y)
//
//  Input:   s = a character string
//  Output:  y = converted value of s,
//           returns 1 if conversion successful, 0 if not
//  Purpose: converts a string to a double precision floating point number.
//
{
    char *endptr;
    *y = strtod(s, &endptr);
    if (*endptr > 0) return(0);
    return(1);
}

//=============================================================================

int  getTokens(char *s)
//
//  Input:   s = a character string
//  Output:  returns number of tokens found in s
//  Purpose: scans a string for tokens, saving pointers to them
//           in shared variable Tok[].
//
//  Notes:   Tokens can be separated by the characters listed in SEPSTR
//           (spaces, tabs, newline, carriage return) which is defined
//           in CONSTS.H. Text between quotes is treated as a single token.
//
{
    int  len, m, n;
    char *c;

    // --- begin with no tokens
    for (n = 0; n < MAXTOKS; n++) Tok[n] = NULL;
    n = 0;

    // --- truncate s at start of comment 
    c = strchr(s,';');
    if (c) *c = '\0';
    len = strlen(s);

    // --- scan s for tokens until nothing left
    while (len > 0 && n < MAXTOKS)
    {
        m = strcspn(s,SEPSTR);              // find token length 
        if (m == 0) s++;                    // no token found
        else
        {
            if (*s == '"')                  // token begins with quote
            {
                s++;                        // start token after quote
                len--;                      // reduce length of s
                m = strcspn(s,"\"\n");      // find end quote or new line
            }
            s[m] = '\0';                    // null-terminate the token
            Tok[n] = s;                     // save pointer to token 
            n++;                            // update token count
            s += m+1;                       // begin next token
        }
        len -= m+1;                         // update length of s
    }
    return(n);
}

//=============================================================================
