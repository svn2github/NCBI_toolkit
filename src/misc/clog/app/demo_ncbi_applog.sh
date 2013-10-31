#!/bin/sh

APPLOG=${1:-"./ncbi_applog"}


Error() {
    echo 'ERROR::' "$1 failed"
    exit 1
}

SetOutput() {
    NCBI_CONFIG__NCBI__NcbiApplogDestination=$1
    export NCBI_CONFIG__NCBI__NcbiApplogDestination
}


#SetOutput cwd


# --- Start

token=`$APPLOG start_app -pid=321 -appname="demo_ncbi_applog" -host=DEMOHOST -sid=DEMOSID`
if [ $? -ne 0 -o -z "$token" ] ; then
    Error "start_app"
fi

# --- Posting

$APPLOG post $token -severity info    -message "info message"    || Error "post(1)"
$APPLOG post $token -severity warning -message "warning message" || Error "post(2)"

# Try to use logging token via $NCBI_APPLOG_TOKEN (the token parameter is empty below)
NCBI_APPLOG_TOKEN=$token
export NCBI_APPLOG_TOKEN

$APPLOG post  '' -message "error message"                 || Error "post(3)"
$APPLOG post  '' -severity trace -message "trace message" || Error "post(4)"
$APPLOG perf  '' -time 1.2                                || Error "perf(1)"
$APPLOG perf  '' -status=404 -time=4.5 -param="k1=1&k2=2" || Error "perf(2)"
$APPLOG extra ''                                          || Error "extra(1)"
$APPLOG extra '' -param="extra1=1"                        || Error "extra(2)"

# --- Request 1

request_token=`$APPLOG start_request '' -sid=request1 -rid=1 -client=1.2.3.4 -param="r11=value1&r12=value2"`
if [ $? -ne 0 -o -z "$request_token" ] ; then
    Error "start_request(1)"
fi
$APPLOG post         $request_token -message "request message" || Error "post(5)"
$APPLOG stop_request $request_token -status=200 -input=11 -output=13 >/dev/null 2>&1 || Error "stop_request(1)"

# --- Posting between requests
$APPLOG post         '' -message "request message" || Error "post(6)"

# --- Request 2
request_token=`$APPLOG start_request '' -sid=request2 -rid=2 -client=5.6.7.8 -param="r21=1&r22=2"`
if [ $? -ne 0 -o -z "$request_token" ] ; then
    Error "start_request(2)"
fi
token=`$APPLOG stop_request $request_token -status=600 -input=21 -output=23` || Error "stop_request(2)"

# --- Stop 
$APPLOG stop_app '' -status=99 || Error "stop_app"


echo "OK"

exit 0
