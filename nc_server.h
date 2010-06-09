/*              Copyright (C) by UCAR
 *
 * File       : $RCSfile: nc_server.h,v $
 * Revision   : $Revision$
 * Directory  : $Source: /code/cvs/pam/isff/src/nc_server/nc_server.h,v $
 * System     : PAM
 * Date       : $Date$
 *
 * Description:
 *
 */

#include <time.h>
#include <signal.h>
#include <vector>
#include <string>
#include <list>
#include <map>
#include <netcdf.hh>

#include "nc_server_rpc.h"
#include <nidas/util/UTime.h>
#include <nidas/util/Logger.h>
#include <nidas/util/Exception.h>

void nc_shutdown(int);

class Connection;
class FileGroup;
class VariableGroup;
class NS_NcFile;
class NS_NcVar;
class Variable;
class OutVariable;

class NcServerApp
{
  public:

    NcServerApp();

    /**
     * Parse the runstring parameters.
     * If the runstring parameters are not OK, then DSMEngine::usage()
     * is called to print a message to stderr, and this method
     * then returns a error value of 1.
     * @return 0: OK, 1: failure.
     */
    void usage(const char *argv0);

    int parseRunstring(int argc, char **argv);

    void setup();

    /** main loop */
    void run();

    static void setupSignals();

    uid_t getUserID()
    {
        return _userid;
    }

    uid_t getGroupID()
    {
        return _groupid;
    }

  private:

    /** Signal handler */
    static void sigAction(int sig, siginfo_t * siginfo, void *vptr);

    std::string _username;

    uid_t _userid;

    gid_t _groupid;

    int _daemon;

    int _logLevel;

};


class BadVariableName:public nidas::util::Exception
{
  public:
    BadVariableName(const char *n):nidas::util::
        Exception("BadVariableName", n)
    {
    }
};

class Connections
{
  private:
    std::vector < Connection * >_connections;
    static Connections *_instance;
  protected:
    Connections(void);
    ~Connections(void);
  public:
    static Connections *Instance();
  public:
    static const int CONNECTIONTIMEOUT;
    int OpenConnection(const struct connection *);
    int CloseConnection(int connectionId);
    int CloseOldConnections();
    Connection *&operator[] (unsigned int);
    unsigned int num() const;
};

class Connection
{
    FileGroup *_filegroup;
    std::string _history;
    int _histlen;

    NS_NcFile *_lastf;          // last file written to, saved for efficiency
    time_t _lastRequest;

  public:
    Connection(const struct connection *);
    ~Connection(void);
    int put_rec(const datarec_float * writerec);
    int put_rec(const datarec_int * writerec);
    int put_history(const std::string &);
    int add_var_group(const struct datadef *);
    time_t LastRequest()
    {
        return _lastRequest;
    }

    const std::string & get_history()
    {
        return _history;
    }

    NS_NcFile *last_file() const;
    void unset_last_file();     // the file has been closed

    class InvalidOutputDir
    {
    };                          // exception class
};

class AllFiles
{
  private:
    std::vector < FileGroup * >_filegroups;
    AllFiles(const AllFiles &); // prevent copying
    AllFiles & operator=(const AllFiles &);     // prevent assignment
    static AllFiles *_instance;
  protected:
    AllFiles(void);
    ~AllFiles(void);
  public:
    static AllFiles *Instance();
    FileGroup *get_file_group(const struct connection *);
    void close();
    void sync();
    void close_old_files(void);
    void close_oldest_file(void);
    int num_files(void) const;

    static void hangup(int sig);
    static void shutdown(int sig);
};

class NS_NcFile:public NcFile
{
  private:
    std::string _fileName;
    double _startTime, _endTime;
    double _interval;
    double _lengthSecs;
    double _timeOffset;         // last timeOffset written
    NcType _timeOffsetType;     // ncFloat or ncDouble
    char _monthLong;            //

    // time tag type, fixed dt or variable?
    enum { FIXED_DELTAT, VARIABLE_DELTAT } _ttType;

    int _timesAreMidpoints;

    NcVar *_baseTimeVar;
    NcVar *_timeOffsetVar;
    std::vector < NS_NcVar ** >_vars;   // for each variable group, a
    // pointer to an array of variables
    std::vector < int > _nvars;  // number of variables in each group
    NcDim *_recdim;
    int _baseTime;
    long _nrecs;

    const char **_dimNames;     // names of requested dimensions
    long *_dimSizes;            // sizes of requested dimensions
    int *_dimIndices;           // position of requested dimensions in variable
    unsigned int _ndims;        // number of dimensions of size > 1
    const NcDim **_dims;        // dimensions to use when creating new vars
    int _ndimalloc;             // allocated size of above arrays
    int _ndims_req;             // number of requested dimensions
    time_t _lastAccess;
    time_t _lastSync;
    std::string _historyHeader;

    NS_NcVar **get_vars(VariableGroup *);
    NS_NcVar *add_var(OutVariable * v);
    NcVar *find_var(OutVariable *);
    int add_attrs(OutVariable * v, NcVar * var);

    NcBool check_var_dims(NcVar *);
    const NcDim *get_dim(NcToken name, long size);

    NS_NcFile(const NS_NcFile &);       // prevent copying
    NS_NcFile & operator=(const NS_NcFile &);   // prevent assignment

  public:
    static const double minInterval;    // if _interval is less than
    // minInterval (most likely 0)
    // then the ttType is VARIABLE_DELTAT

    NS_NcFile(const std::string &, enum FileMode, double, double, double);
    ~NS_NcFile(void);

    const std::string & getName() const;

    int operator <(const NS_NcFile & x)
    {
        return _startTime < x._startTime;
    }
    friend int operator <(const NS_NcFile & x, const NS_NcFile & y)
    {
        return x._startTime < y._startTime;
    }

    inline int StartTimeLE(double time) const
    {
#ifdef DEBUG
        DLOG(("") << "start=" << nidas::util::UTime(_startTime) <<
             ", end=" << nidas::util::UTime(_endTime) <<
             ", current=" nidas::util::UTime(time));
#endif
        return (_startTime <= time);
    };
    inline int EndTimeLE(double time) const
    {
#ifdef DEBUG
        DLOG(("") << "start=" << nidas::util::UTime(_startTime) <<
             ", end=" << nidas::util::UTime(_endTime) <<
             ", current=" nidas::util::UTime(time));
#endif
        return (_endTime <= time);
    }
    inline int EndTimeGT(double time) const
    {
#ifdef DEBUG
        DLOG(("") << "start=" << nidas::util::UTime(_startTime) <<
             ", end=" << nidas::util::UTime(_endTime) <<
             ", current=" nidas::util::UTime(time));
#endif
        return (_endTime > time);
    }
    NcBool put_rec(const struct datarec_float *, VariableGroup *,
                   double dtime);
    NcBool put_rec(const struct datarec_int *, VariableGroup *,
                   double dtime);
    long put_time(double, const char *);
    int put_history(std::string history);
    time_t LastAccess() const
    {
        return _lastAccess;
    }
    NcBool sync(void);

    class NetCDFAccessFailed
    {
    };                          // exception class
};

// A file group is a list of similarly named files with the same
// time series data interval and length
class FileGroup
{
  private:

    std::vector < Connection * >_connections;

    std::list < NS_NcFile * >_files;    // List of files in this group

    static const int FILEACCESSTIMEOUT;
    static const int MAX_FILES_OPEN;

    FileGroup(const FileGroup &);       // prevent copying
    FileGroup & operator=(const FileGroup &);   // prevent assignment

    std::string _outputDir;
    std::string _fileNameFormat;
    std::string _CDLFileName;
    std::vector < VariableGroup * >_vargroups;
    double _interval;
    double _fileLength;

  public:

    FileGroup(const struct connection *);
    ~FileGroup(void);

    NS_NcFile *put_rec(const datarec_float * writerec, NS_NcFile * f);
    NS_NcFile *put_rec(const datarec_int * writerec, NS_NcFile * f);

    int match(const std::string & dir, const std::string & file);
    NS_NcFile *get_file(double time);
    NS_NcFile *open_file(double time);
    void close();
    void sync();
    void close_old_files(void);
    void close_oldest_file(void);
    void add_connection(Connection *);
    void remove_connection(Connection *);
    int add_var_group(const struct datadef *);
    double interval() const
    {
        return _interval;
    }
    double length() const
    {
        return _fileLength;
    }
    int active() const
    {
        return _connections.size() > 0;
    }
    int num_files() const
    {
        return _files.size();
    }
    int num_var_groups() const
    {
        return _vargroups.size();
    }
    time_t oldest_file();

    std::string build_name(const std::string & outputDir,
                           const std::string & nameFormat, double dtime,
                           double fileLength) const;
    int check_file(const std::string &) const;
    int ncgen_file(const std::string &, const std::string &) const;

    class InvalidInterval
    {
    };
    class InvalidFileLength
    {
    };

};

class VariableGroup
{
  private:
    double _interval;
    std::vector < Variable * >_invars;
    std::vector < OutVariable * >_outvars;
    unsigned int _ndims;        // number of dimensions
    long *_dimsizes;            // dimension sizes
    char **_dimnames;           // dimension names
    int _nsamples;              // number of samples per time record
    int _nprefixes;
    NS_rectype _rectype;
    NS_datatype _datatype;
    int _fillMissing;
    float _floatFill;
    int _intFill;
    int _ngroup;

    void check_counts_variable();


    VariableGroup(const VariableGroup &);       // prevent copying
    VariableGroup & operator=(const VariableGroup &);   // prevent assignment
  public:
    VariableGroup(const struct datadef *, int n, double finterval);
    ~VariableGroup(void);

    int ngroup() const
    {
        return _ngroup;
    }

    OutVariable *get_var(int n) const
    {
        return _outvars[n];
    }

    int same_var_group(const struct datadef *) const;
    int similar_var_group(const VariableGroup *) const;
    // void append_to_suffix(char);
    // void set_suffix(const char*);
    const char *suffix() const;
    void create_outvariables(void) throw(BadVariableName);

    OutVariable *createCountsVariable(const std::string& name)
        throw(BadVariableName);

    char *counts_name(char *);

    double interval() const
    {
        return _interval;
    }
    int num_samples() const
    {
        return _nsamples;
    }
    int num_vars() const
    {
        return _outvars.size();
    }
    NS_rectype rec_type() const
    {
        return _rectype;
    }
    NS_datatype data_type() const
    {
        return _datatype;
    }

    int NumCombTrivar(int n) const;

    int num_dims() const;
    long dim_size(unsigned int i) const;
    const char *dim_name(unsigned int i) const;

    float floatFill() const
    {
        return _floatFill;
    }
    int intFill() const
    {
        return _intFill;
    }

};

//
class NS_NcVar
{
    NcVar *_var;
    /**
     * for requested dimensions, their position in the NetCDF variable's dimensions
     */
    int *_dimIndices;

    /**
     * length of _dimIndices. This is 2 + number of requested dimensions
     */
    int _ndimIndices;

    long *_start;
    long *_count;
    int _isCnts;
    float _floatFill;
    int _intFill;

  public:
    NS_NcVar(NcVar *, int *dimIndices, int ndims_group, float ffill,
             int lfill, int isCnts = 0);
    ~NS_NcVar();
    NcVar *var() const
    {
        return _var;
    }
    NcBool set_cur(long, int, const long *);
    int put(const float *d, const long *);
    int put(const int * d, const long *);
    int put_len(const long *);
    const char *name() const
    {
        return _var->name();
    }
    NcAtt *get_att(NcToken attname) const
    {
        return _var->get_att(attname);
    }
    NcBool add_att(NcToken attname, const char *v) const
    {
        return _var->add_att(attname, v);
    }
    int &isCnts()
    {
        return _isCnts;
    }
    float floatFill() const
    {
        return _floatFill;
    }
    int intFill() const
    {
        return _intFill;
    }

};

class Variable
{
  protected:
    std::string _name;

    /**
     * If this variable is referenced as a counts variable by other variables.
     */
    int _isCnts;

    std::map <std::string, std::string> _strAttrs;

    Variable & operator=(const Variable &);     // prevent assignment

  public:
    Variable(const std::string &n);

    Variable(const Variable &);

    virtual ~ Variable(void) {}

    int &isCnts()
    {
        return _isCnts;
    }
    const std::string& name() const
    {
        return _name;
    }
    const std::string& units() const
    {
        return att_val("units");
    }

    void set_name(const std::string&);

    void add_att(const std::string& name, const std::string& val);

    std::vector<std::string> get_attr_names() const;

    const std::string& att_val(const std::string& n) const;

};

class OutVariable:public Variable
{

    NS_datatype _datatype;

    OutVariable *_countsvar;

    float _floatFill;

    int _intFill;

    OutVariable & operator=(const OutVariable &);       // prevent assignment

    OutVariable(const OutVariable &);   // prevent copying

  public:

    OutVariable(const Variable &, NS_datatype, float, int)
        throw(BadVariableName);

    NS_datatype data_type() const
    {
        return _datatype;
    }
    OutVariable *&counts_variable()
    {
        return _countsvar;
    }

    float floatFill() const
    {
        return _floatFill;
    }
    int intFill() const
    {
        return _intFill;
    }

};

