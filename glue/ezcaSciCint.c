/* $Id: ezcaSciCint.c,v 1.5 2004/01/06 20:37:06 till Exp $ */

/* SCILAB C-interface to ezca / multiEzca */
#include <mex.h>
#include "stack-c.h"
#include <string.h>
#include <cadef.h>
#include <ezca.h>
#include <multiEzca.h>

extern void C2F(cts_stampf_)();

#if 0
#define getRhsVar(n,ct,mx,nx,lx) \
	( C2F(getrhsvar)((c_local=n,&c_local),ct,mx,nx,(integer *) lx,1L) )
#define checkRhs(minrhs,maxrhs)  \
	( C2F(checkrhs)(fname,(c_local = minrhs,&c_local),(c1_local=maxrhs,&c1_local),\
              strlen(fname)) )
#define checkLhs(minlhs,maxlhs)  \
	( C2F(checklhs)(fname,(c_local = minlhs,&c_local),(c1_local=maxlhs,&c1_local),\
              strlen(fname)) )
#define checkRow(pos,m,n) \
	( check_row(pos,m,n) )
#define checkColumn(pos,m,n) \
	( check_column(pos,m,n) )
#endif

/* there's a typo in the scilab header :-( */
#define check_column check_col

/* convert a short array into an int array */
static void
s2iInPlace(void *buf, int m)
{
int i;
	for (i = m-1; i>=0; i--) {
		((int*)buf)[i] = (int) ((short*)buf)[i];
	}
}

static int
arg2ezcaType(char *pt, int idx)
{
int m,n;
char **s = 0;

	if ( Rhs < idx )
		return 0;

	GetRhsVar(idx, "S", &m, &n, &s);
	CheckRow(idx, m, n);

	switch ( toupper((*s)[0]) ) {
			case 'D': *pt = ezcaDouble; break;
			case 'F': *pt = ezcaFloat;  break;
			case 'L': *pt = ezcaLong;   break;
			case 'S': *pt = ezcaShort;  break;
			case 'B': *pt = ezcaByte;   break;
			case 'C': *pt = ezcaString; break;
			case 'N': *pt = ezcaNative; break;
                                                                                            
			default:
					cerro("Invalid type - must be 'byte', 'short', 'long', 'float', 'double' or 'char'");
			return 0;
	}

	return 1;
}

static int intsezcaGet(char *fname)
{
int mpvs, mtmp, ntmp, itmp, jtmp;
void *buf       = 0;
TS_STAMP  *ts   = 0;
char  **pvs;
int	n    = 0;
char   type  = ezcaNative;

	CheckRhs(1,3);
	CheckLhs(1,2);

 	GetRhsVar(1,"S",&mpvs,&ntmp,&pvs);
 	CheckColumn(1,mpvs,ntmp);

	if ( Rhs > 1 ) {
		GetRhsVar(2, "i", &mtmp, &ntmp, &itmp);
		CheckScalar(2, mtmp, ntmp);
		n = *istk(itmp);
		if ( Rhs > 2 && !arg2ezcaType(&type,3) )
			return 0;
	}

	ntmp = 1;
	CreateCVar(Rhs + 1,"d",&ntmp,&mpvs,&ntmp,&itmp,&jtmp);

	/* we can't preallocate the result matrix -- n is unknown at this point */

	if ( !multi_ezca_get( pvs, &type, &buf, mpvs, &n, &ts ) ) {
		for ( itmp = 1; itmp <= Lhs; itmp++ )
			LhsVar(itmp) = 0; 
		return 0;
	}

	/* NOTE: if CreateVarxxx fails, there is a memory leak
	 *       (macro returns without chance to clean up)
	 */
	if ( Lhs >= 1 ) {
		if ( ezcaString == type ) {
			CreateVarFromPtr(Rhs + 2, "S", &mpvs, &n, buf);
		} else {
			CreateVarFromPtr(Rhs + 2, "d", &mpvs, &n, &buf);
		}
		LhsVar(1)=Rhs + 2;

		if ( Lhs >= 2 ) {
			multi_ezca_ts_cvt( mpvs, ts, stk(itmp), stk(jtmp) );
			LhsVar(2)=Rhs + 1;
		}
	}

	if ( buf ) {
		if ( ezcaString == type ) {
			FreeRhsSVar(((char**)buf));
		} else {
			FreePtr(&buf);
		}
 	}

	if ( ts ) {
		FreePtr( &ts );
	}

	return 0;
}

static int intsezcaPut(char *fname)
{
int mpvs, mval, ntmp, n, i;
void *buf;
char **pvs;
char type  = ezcaNative;

	CheckRhs(2,3);
	CheckLhs(1,1);

	GetRhsVar(1,"S",&mpvs,&ntmp,&pvs);
	CheckColumn(1,mpvs,ntmp);

	if ( 10 == VarType(2) ) {
	GetRhsVar(2,"S",&mval,&n,&buf);
	type = ezcaString;
	} else {
	GetRhsVar(2,"d",&mval,&n,&i);
	buf = stk(i);
	}

	if ( Rhs > 2 ) {
		char t;
		if ( !arg2ezcaType( &t, 3 ) )
			return 0;
		if ( (ezcaString == type) != (ezcaString == t) )  {
			cerro("string value type conversion not implemented, sorry");
			return 0;
		}
		type = t;
	}

	/* default value to optional argument type */
	/* cross variable size checking */
	multi_ezca_put(pvs, mpvs, type, buf, mval, n);

	LhsVar(1)=0;
	return 0;
}

int intsezcaGetNelem(char *fname)
{
int m,n,i;
char  **pvs;

	CheckRhs(1,1);
	CheckLhs(1,1);

	GetRhsVar(1,"S",&m,&n,&pvs);
	CheckColumn(1,m,n);

	CreateVar(2,"i",&m,&n,&i);
	if ( !multi_ezca_get_nelem(pvs, m, istk(i)) )
 		LhsVar(1)=2;
 	return 0;
}

int intsezcaGetControlLimits(char *fname)
{
int m,n,d1,d2;
char **pvs;
MultiArgRec args[2];

	CheckRhs(1,1);
	CheckLhs(1,2);

	GetRhsVar(1,"S",&m,&n,&pvs);
	CheckColumn(1,m,n);

	CreateVar(2, "d", &m, &n, &d1);
	CreateVar(3, "d", &m, &n, &d2);

	MSetArg(args[0], sizeof(double), stk(d1), 0); 
	MSetArg(args[1], sizeof(double), stk(d2), 0); 

	if ( multi_ezca_get_misc(pvs, m, (MultiEzcaFunc)ezcaGetControlLimits, NumberOf(args), args) ) {
		for ( n=1; n<=Lhs; n++ )
			LhsVar(n) = Rhs + n;
	}
	
	return 0;
}

int intsezcaGetGraphicLimits(char *fname)
{
int m,n,d1,d2;
char **pvs;
MultiArgRec args[2];

	CheckRhs(1,1);
	CheckLhs(1,2);

	GetRhsVar(1,"S",&m,&n,&pvs);
	CheckColumn(1,m,n);

	CreateVar(2, "d", &m, &n, &d1);
	CreateVar(3, "d", &m, &n, &d2);

	MSetArg(args[0], sizeof(double), stk(d1), 0); 
	MSetArg(args[1], sizeof(double), stk(d2), 0); 

	if ( multi_ezca_get_misc(pvs, m, (MultiEzcaFunc)ezcaGetGraphicLimits, NumberOf(args), args) ) {
		for ( n=1; n<=Lhs; n++ )
			LhsVar(n) = Rhs + n;
	}
	
	return 0;
}

int intsezcaGetStatus(char *fname)
{
TS_STAMP *ts;
int      hasImag, m,n,i,j;
char     **pvs;

MultiArgRec args[3];

	CheckRhs(1,1);
	CheckLhs(1,3);
	GetRhsVar(1,"S",&m,&n,&pvs);
	CheckColumn(1,m,n);

	MSetArg(args[0], sizeof(TS_STAMP), 0,       &ts); /* timestamp */

 	CreateVar(2,"i",&m,&n,&i);
	MSetArg(args[1], sizeof(short),    istk(i), 0  ); /* status    */

 	CreateVar(3,"i",&m,&n,&i);
	MSetArg(args[2], sizeof(short),    istk(i), 0  ); /* severity  */

	hasImag = 1;
 	CreateCVar(4,"d",&hasImag,&m,&n,&i,&j);

	if ( multi_ezca_get_misc(pvs, m, (MultiEzcaFunc)ezcaGetStatus, NumberOf(args), args) ) {

		s2iInPlace(args[2].buf, m);
 		LhsVar(1)=3;

		if ( Lhs >= 2 ) {
			s2iInPlace(args[1].buf, m);
 			LhsVar(2)=2;
			
			if ( Lhs >= 3 ) {
				LhsVar(3)=4;
				multi_ezca_ts_cvt( m, ts, stk(i), stk(j) );
			}
		}
	}
	if ( ts )
		FreePtr( &ts );

 	return 0;
}

int intsezcaGetPrecision(char *fname)
{
int m,n,i;
char  **pvs;
MultiArgRec	args[1];

	CheckRhs(1,1);
	CheckLhs(1,1);
	GetRhsVar(1,"S",&m,&n,&pvs);
	CheckColumn(1,m,n);

	CreateVar(2,"i",&m,&n,&i);

	MSetArg(args[0], sizeof(short), istk(i), 0);

	if ( multi_ezca_get_misc(pvs, m, (MultiEzcaFunc)ezcaGetPrecision, NumberOf(args), args) ) {
		s2iInPlace(istk(i),m);
		LhsVar(1) = 2;
	}
 	return 0;
}

typedef char units_string[EZCA_UNITS_SIZE];


int intsezcaGetUnits(char *fname)
{
int  m,n,i;
char **pvs,**tmp;
units_string *strbuf = 0;
MultiArgRec  args[1];

	CheckRhs(1,1);
	CheckLhs(1,1);
	GetRhsVar(1,"S",&m,&n,&pvs);
	CheckColumn(1,m,n);

	MSetArg(args[0], sizeof(units_string), 0, &strbuf);

	if ( multi_ezca_get_misc(pvs, m, (MultiEzcaFunc)ezcaGetUnits, NumberOf(args), args) &&
		 /* scilab expects NULL terminated char** list */
		 (tmp = malloc(sizeof(char*) * (m+1))) ) {
		for ( i=0; i<m; i++ ) {
			tmp[i] = strbuf[i];
		}
		tmp[m] = 0;
 		CreateVarFromPtr(2, "S",&m,&n,tmp);
		FreePtr(&tmp);
 		LhsVar(1)= 2;
	}
 	FreePtr(&strbuf);
 	return 0;
}


int intsezcaGetRetryCount(char *fname)
{
int m=1,i;

	CheckRhs(0,0);
	CheckLhs(1,1);
	CreateVar(1,"i",&m,&m,&i);
	*istk(i) = ezcaGetRetryCount();
	LhsVar(1)= 1;
	return 0;
}

int intsezcaSetRetryCount(char *fname)
{
int m,n,i;

	CheckRhs(1,1);
	CheckLhs(1,1);
	GetRhsVar(1,"i",&m,&n,&i);
	CheckScalar(1,m,n);
	ezcaSetRetryCount(*istk(i));
	LhsVar(1)=0;
	return 0;
}

int intsezcaGetTimeout(char *fname)
{
int m=1,i;

	CheckRhs(0,0);
	CheckLhs(1,1);
	CreateVar(1,"r",&m,&m,&i);/* named: res */
	*sstk(i) = ezcaGetTimeout();
	LhsVar(1)= 1;
	return 0;
}


int intsezcaSetTimeout(char *fname)
{
int m,n,i;
	CheckRhs(1,1);
	CheckLhs(1,1);
	GetRhsVar(1,"r",&m,&n,&i);
	CheckScalar(1,m,n);
	ezcaSetTimeout(*sstk(i));
	LhsVar(1)=0;
	return 0;
}

int intsezcaDebugOn(char *fname)
{
	CheckRhs(0,0);
	CheckLhs(1,1);
	ezcaDebugOn();
	LhsVar(1)=0;
	return 0;
}

int intsezcaDebugOff(char *fname)
{
	CheckRhs(0,0);
	CheckLhs(1,1);
	ezcaDebugOff();
	LhsVar(1)=0;
	return 0;
}

int intsezcaSetSeverityWarnLevel(char *fname)
{
int m,n,i;
	CheckRhs(1,1);
	CheckLhs(1,1);
	GetRhsVar(1,"i",&m,&n,&i);
	CheckScalar(1,m,n);
	ezcaSeverityWarnLevel = *istk(i);
	LhsVar(1)=0;
	return 0;
}

int intsecdrGet(char *fname)
{
long *buf;
extern void C2F(ecdrget)(char *, int *, long **, int *);

int m,n,i,nord;

	CheckRhs(1,1);
	CheckLhs(1,1);
	GetRhsVar(1,"c",&m,&n,&i);
	CheckRow(1,m,n);
	C2F(ecdrget)(cstk(i),&n,&buf,&nord);
	CreateVarFromPtr(2,"i",&m,&nord,&buf);
	if (buf) {
 		LhsVar(1)=2;
		FreePtr(&buf);
	}
 return 0;
}

static GenericTable Tab[]={
  {(Myinterfun)sci_gateway, intsezcaGet,					"lcaGet"},
  {(Myinterfun)sci_gateway, intsezcaPut,					"lcaPut"},
  {(Myinterfun)sci_gateway, intsezcaGetNelem,				"lcaGetNelem"},
  {(Myinterfun)sci_gateway, intsezcaGetControlLimits,		"lcaGetControlLimits"},
  {(Myinterfun)sci_gateway, intsezcaGetGraphicLimits,		"lcaGetGraphicLimits"},
  {(Myinterfun)sci_gateway, intsezcaGetStatus,				"lcaGetStatus"},
  {(Myinterfun)sci_gateway, intsezcaGetPrecision,			"lcaGetPrecision"},
  {(Myinterfun)sci_gateway, intsezcaGetUnits,				"lcaGetUnits"},
  {(Myinterfun)sci_gateway, intsezcaGetRetryCount,			"lcaGetRetryCount"},
  {(Myinterfun)sci_gateway, intsezcaSetRetryCount,			"lcaSetRetryCount"},
  {(Myinterfun)sci_gateway, intsezcaGetTimeout,				"lcaGetTimeout"},
  {(Myinterfun)sci_gateway, intsezcaSetTimeout,				"lcaSetTimeout"},
  {(Myinterfun)sci_gateway, intsezcaDebugOn,				"lcaDebugOn"},
  {(Myinterfun)sci_gateway, intsezcaDebugOff,				"lcaDebugOff"},
  {(Myinterfun)sci_gateway, intsezcaSetSeverityWarnLevel,	"lcaSetSeverityWarnLevel"},
  {(Myinterfun)sci_gateway, intsecdrGet,					"lecdrGet"},
};
                                                                                            
int
C2F(ezca)()
{
  Rhs = Max(0, Rhs);
  (*(Tab[Fin-1].f))(Tab[Fin-1].name,Tab[Fin-1].F);
  return 0;
}
