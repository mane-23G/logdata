# logdata
This is a function that print various statistics of login data of one or more users by reading a kernel data structure (the wtmp file). This was tested on an eniac server that held the computer science deparments student accounts.

### Build with
gcc -o logdata logdata.c

### Usage 
logdata \[-a -s -f \<file\>\] username ...
<br>
The options are: <br>
-a : Show the log times for all users that have entries in the file _PATH_WTMP. <br>
-s : Produce a line of output at the end showing the totals for all users listed. <br>
-f \<file\> : Use <file> instead of _PATH_WTMP. <br>
### Features<br>
-If no username is given, the time for the current user is displayed<br>
-If the total time is less than a day, the days field is omitted<br>
-If it is less than an hour, the hours and days are omitted <br>
-If it’s less than a minute, only the seconds are displayed<br>
-If a username is given but there are no logins for the user, "0 seconds" is listed for that username<br>
-If any value is zero, the units for that value are not displayed<br>
<br>
### Defects/Shortcomings
-The time that the user is logged in is always inconsisent and not correct not sure why. 

<br>
This was last tested on 11/18/24 and was working as intended (with the stated defects).

Thank you to Professor Weiss for the assigment!
