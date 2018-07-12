Mahimahi record the objects in protobuf files. During replay it matches request with recorded objects (all objects in the recorded directory) and compute a match score. If a match is successful, response is successful. Otherwise, 404 is sent.

I have added a few lines to drop objects in replayserver.cc to match object we want to drop and send 404 in response. I have manually tried this with a few objects and its working.

The code file that controls this is: mahimahi/src/frontend/replayserver.cc

We can drop any object we want using this simple trick. I dont think any other part of code would need any modifications.


-- To install
Clone this repo and cd to dir.
./autogen.sh
./configure --prefix=*some_dir* (Better to use a directory other than system's bin. I used "temp-ins".)
make
sudo make install
mahimahi binaries will be in *some_dir*/bin/

-- Record a website
*some_dir*/bin/mm-webrecord *recorded-sites-dir*/google chromium-browser --ignore-certificate-errors --user-data-dir=/tmp/nonexistent$(date +%s%N) www.google.com

--Replay the website
*some_dir*/bin/mm-webreplay recorded-sites/google/ chromium-browser --ignore-certificate-errors --user-data-dir=/tmp/nonexistent$(date +%s%N) www.google.com

Objects loaded and their drop status will be recorded in /tmp/mm-log.txt
