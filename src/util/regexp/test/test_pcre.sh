#! /bin/sh

# This file is generated by configure from RunTest.in. Make any changes
# to that file.

# Run PCRE tests

cf=diff
testdata=./testdata
pcre=./test_regexp

# Select which tests to run; if no selection, run all

do1=no
do2=no
do3=no
do4=no
do5=no
do6=no

while [ $# -gt 0 ] ; do
  case $1 in
    1) do1=yes;;
    2) do2=yes;;
    3) do3=yes;;
    4) do4=yes;;
    5) do5=yes;; 
    6) do6=yes;; 
    *) echo "Unknown test number $1"; exit 1;;
  esac
  shift
done

if [ "@UTF8@" = "" ] ; then
  if [ $do5 = yes ] ; then
    echo "Can't run test 5 because UFT8 support is not configured"
    exit 1
  fi   
  if [ $do6 = yes ] ; then
    echo "Can't run test 6 because UFT8 support is not configured"
    exit 1
  fi   
fi    

if [ $do1 = no -a $do2 = no -a $do3 = no -a $do4 = no -a\
     $do5 = no -a $do6 = no ] ; then
  do1=yes
  do2=yes
  do3=yes
  do4=yes
  if [ "@UTF8@" != "" ] ; then do5=yes; fi
  if [ "@UTF8@" != "" ] ; then do6=yes; fi
fi

# Primary test, Perl-compatible

if [ $do1 = yes ] ; then
  echo "Testing main functionality (Perl compatible)"
  $pcre $testdata/testinput1 testtry
  if [ $? = 0 ] ; then
    $cf testtry $testdata/testoutput1
    if [ $? != 0 ] ; then exit 1; fi
  else exit 1
  fi
fi

# PCRE tests that are not Perl-compatible - API & error tests, mostly

if [ $do2 = yes ] ; then
  echo "Testing API and error handling (not Perl compatible)"
  $pcre -i $testdata/testinput2 testtry
  if [ $? = 0 ] ; then
    $cf testtry $testdata/testoutput2
    if [ $? != 0 ] ; then exit 1; fi
  else exit 1
  fi
fi

# Additional Perl-compatible tests for Perl 5.005's new features

if [ $do3 = yes ] ; then
  echo "Testing Perl 5.005 features (Perl 5.005 compatible)"
  $pcre $testdata/testinput3 testtry
  if [ $? = 0 ] ; then
    $cf testtry $testdata/testoutput3
    if [ $? != 0 ] ; then exit 1; fi
  else exit 1
  fi
fi

if [ $do1 = yes -a $do2 = yes -a $do3 = yes ] ; then
  echo " " 
  echo "The three main tests all ran OK"
  echo " " 
fi

# Locale-specific tests, provided the "fr" locale is available

if [ $do4 = yes ] ; then
  locale -a | grep '^fr$' >/dev/null
  if [ $? -eq 0 ] ; then
    echo "Testing locale-specific features (using 'fr' locale)"
    $pcre $testdata/testinput4 testtry
    if [ $? = 0 ] ; then
      $cf testtry $testdata/testoutput4
      if [ $? != 0 ] ; then 
        echo " "
        echo "Locale test did not run entirely successfully."
        echo "This usually means that there is a problem with the locale"
        echo "settings rather than a bug in PCRE."    
      else
      echo "Locale test ran OK" 
      fi 
      echo " " 
    else exit 1
    fi
  else
    echo "Cannot test locale-specific features - 'fr' locale not found,"
    echo "or the \"locale\" command is not available to check for it."
    echo " " 
  fi
fi

# Additional tests for UTF8 support

if [ $do5 = yes ] ; then
  echo "Testing experimental, incomplete UTF8 support (Perl compatible)"
  $pcre $testdata/testinput5 testtry 
  if [ $? = 0 ] ; then
    $cf testtry $testdata/testoutput5
    if [ $? != 0 ] ; then exit 1; fi
  else exit 1
  fi
  echo "UTF8 test ran OK"
  echo " "
fi

if [ $do6 = yes ] ; then
  echo "Testing API and internals for UTF8 support (not Perl compatible)"
  $pcre $testdata/testinput6 testtry 
  if [ $? = 0 ] ; then
    $cf testtry $testdata/testoutput6
    if [ $? != 0 ] ; then exit 1; fi
  else exit 1
  fi
  echo "UTF8 internals test ran OK"
  echo " "
fi

# End
