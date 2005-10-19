#! /bin/sh
# $Id$

# DBLIB does not work (on Linux at least) when this limit is > 1024
ulimit -n 1024 > /dev/null 2>&1

# Declare drivers and servers
driver_list="ctlib dblib ftds odbc" # msdblib, mysql
server_list="MS_DEV1 STRAUSS MOZART"
server_mssql="MS_DEV1"

res_file="/tmp/$0.$$"
trap 'rm -f $res_file' 1 2 15

n_ok=0
n_err=0
sum_list=""

# We need this to find dbapi_driver_check
PATH="$CFG_BIN:$PATH"
export PATH

# Run one test
RunTest()
{
  echo
  (
    $CHECK_EXEC dbapi_sample $1 > $res_file 2>&1
  )
  if test $? -eq 0 ; then
      echo "OK:"
      n_ok=`expr $n_ok + 1`
      sum_list="$sum_list XXX_SEPARATOR +  dbapi_sample $1 "
      return
  fi

  # error occurred
  n_err=`expr $n_err + 1`
  sum_list="$sum_list XXX_SEPARATOR -  dbapi_sample $1 "

  cat $res_file
}

# Check existence of the "dbapi_driver_check"
$CHECK_EXEC dbapi_driver_check
if test $? -ne 99 ; then
  echo "The DBAPI driver existence check application not found."
  echo
  exit 1
fi

# Loop through all combinations of {driver, server, test}
for driver in $driver_list ; do
  cat <<EOF

******************* DRIVER:  $driver ************************
EOF

  if $CHECK_EXEC dbapi_driver_check  $driver ; then
    for server in $server_list ; do
      if test \( $driver = "ctlib" -o $driver = "dblib" \) -a $server = $server_mssql ; then
         continue
      fi
      if test \( $driver = "odbc" -o $driver = "msdblib" \) -a  $server != $server_mssql ; then
         continue
      fi
      if test $driver = "ftds" -a  $server != $server_mssql ; then
         continue
      fi

      cat <<EOF

~~~~~~ SERVER:  $server ~~~~~~~~~~~~~~~~~~~~~~~~
EOF

     RunTest "-d $driver -s $server"
    done

  else
    cat <<EOF

Driver not found.
EOF
  fi
done

rm -f $res_file


# Print summary
cat <<EOF


*******************************************************

SUCCEEDED:  $n_ok
FAILED:     $n_err
EOF
echo "$sum_list" | sed 's/XXX_SEPARATOR/\
/g'


# Exit
exit $n_err

