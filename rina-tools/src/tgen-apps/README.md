# Traffic generation applications developed within the ERASER project

## Overview

tgen-apps is a simple client/server application in which a server registers to one or more DIFs, 
waits for flow allocation requests and then for each flow either: 

* logs every single SDU received (dump server) 
* drops the SDUs received silently (drop server) or
* displays statistics about the data read from the flow (log server) 

There are a number of clients that generate different types of traffic:

* data: constant traffic at a fixed rate
* poisson: traffic following a poisson distribution
* exponential: traffic following a exponential distribution
* onoff: traffic source with an ON/OFF behaviour
* video: traffic source emulating video traffic
* voice: traffic source emulating voice traffic

## Server usage

    USAGE: 

       ./tg-server  [-t <string>] [-v] [-i <string>] [-n <string>] [--]
                    [--version] [-h] <string> ...


    Where: 

       -t <string>,  --type <string>
         Server type (options = log, dump, drop), default = log

       -v,  --verbose
         display more debug output, default = false

       -i <string>,  --instance <string>
         Application process instance, default = 1

       -n <string>,  --name <string>
         Application process name, default = TgServer

       --,  --ignore_rest
         Ignores the rest of the labeled arguments following this flag.

       --version
         Displays version information and exits.

       -h,  --help
         Displays usage information and exits.

       <string>  (accepted multiple times)
         DIFs to use, empty for any DIF

## Client usage

    USAGE: 

       ./tg-client  [-f <int>] [-u <unsigned int>] [-U <int>] [-z <float>] [-l
                    <double>] [-F <int>] [-O <int>] [-t <string>] [-L <unsigned
                    int>] [-E <unsigned int>] [-s <int>] [-v] [-D <string>] [-M
                    <float>] [-S <unsigned int>] [-d <int>] [-q <int>] [-I
                    <int>] [-Q <string>] [-j <string>] [-m <string>] [-i
                    <string>] [-n <string>] [--] [--version] [-h]

There are some arguments that can be set for all clients and some arguments that are client-specific. The common arguments are:

      -t <string>,  --type <string>
        client type (options=data,exp,video,voice,poisson,onoff), default =
        data

      -L <unsigned int>,  --loss <unsigned int>
        Max loss in NUM/10000, deafult = 10000

      -E <unsigned int>,  --delay <unsigned int>
        Max delay in microseconds, deafult = 0

      -s <int>,  --max_sdu_size <int>
        max sdu size, default = 1400

      -v,  --verbose
        display more debug output, default = false

      -D <string>,  --dif <string>
        DIF to use, empty for any DIF, default = ""

      -d <int>,  --duration <int>
        Test duration in s, default = 10

      -j <string>,  --sinstance <string>
        Server process instance, default = 1

      -m <string>,  --sname <string>
        Server process name, default = TgServer

      -i <string>,  --instance <string>
        Application process instance, default = 1

      -n <string>,  --name <string>
        Application process name, default = Exponential

      --,  --ignore_rest
        Ignores the rest of the labeled arguments following this flag.

      --version
        Displays version information and exits.

      -h,  --help
        Displays usage information and exits.

Arguments specific to the *data*, *exp* and *video* clients:

      -M <float>,  --Mbps <float>
        Average Rate in Mbps, default = 10.0

      -S <unsigned int>,  --pSize <unsigned int>
        Packet size in B, default 1000B

Arguments specific to the *poisson* client:

      -M <float>,  --Mbps <float>
        Average Rate in Mbps, default = 10.0

      -S <unsigned int>,  --pSize <unsigned int>
        Packet size in B, default 1000B

      -l <double>,  --load <double>
        Average flow load, default 1.0

Arguments specific to the *onoff* client:

      -M <float>,  --Mbps <float>
        Average Rate in Mbps, default = 10.0

      -S <unsigned int>,  --pSize <unsigned int>
        Packet size in B, default 1000B

      -O <int>,  --dOn <int>
        Average duration of On interval in ms, default 1000 ms

      -F <int>,  --dOff <int>
        Average duration of Off interval in ms, default 1000 ms

Arguments specific to the *voice* client:

      -S <unsigned int>,  --pSize <unsigned int>
        Packet size in B, default 1000B

      -z <float>,  --hz <float>
        Frame Rate flow, default = 50.0Hz

      -O <int>,  --dOn <int>
        Average duration of On interval in ms, default 1000 ms

      -U <int>,  --vOn <int>
        Variation of duration of On interval in ms, default 1000 ms

      -u <unsigned int>,  --fSize <unsigned int>
        Packet size in B when OFF, default 1000B

      -F <int>,  --dOff <int>
        Average duration of Off interval in ms, default 1000 ms

      -f <int>,  --vOff <int>
        Variation of duration of Off interval in ms, default 1000 ms
