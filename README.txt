Mateo Zoto
10082263
CPSC 441
=============
HOW TO COMPILE:
=============
Assuming you are on the root folder of proxy.c, run the following
command in your terminal:
gcc proxy.c -o proxy.o

Compiling was done strictly on Unix and WILL NOT work on a Windows based machine.
=============
HOW TO RUN
=============
./proxy.o [Random port between 8000-8999 will be generated]
./proxy.o PORT

If the port isn't valid, the program will autocorrect it [echoserver feature]
Since this was based of the original echoserver, the binding error exists.
=============
TESTING CONDITIONS
=============
All my tests were conducted on Internet Explorer 11 on Windows 8.1.
Proxy was run on the csc.ucalgary.ca server via SSH to conduct testing of the proxy.

All the sample pages from the assignment spec give output as expected by the assignment such as
even pages and images being passed through and returned to the client and
odd pages being blocked with odd images being blocked if the page is even. Eventest3 which is
the most heavy duty test passes with flying colors and will replace odd content with the trollface
as demonstrated in lecture.

In regards to more complicated websites...

ucalgary.ca loads most of the content will all the odd content blocked out
accordingly. Websites like a couple of vBulletin forums, goal.com/en, engadget
load fine and odd content does get blocked accordingly. Some of these more complex
sites do require a refresh sometimes to push through.

More complex websites like google.com, facebook etc, mainly websites that consist of chunked data
do not work whatsoever and spit out HTTP errors to the browser. 

In regards to features, no extra features were introduced that weren't in the assignment spec, but
redirection to an error page when content is odd works or to a troll face when content is odd sized.

The console itself displays HTTP header information for both incoming and outgoing, which host it's connecting to.

One error that I wish I could have fixed was the server showing 0.0.0.0 for the IP of the client and I was not able to
figure out.