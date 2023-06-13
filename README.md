# Capstone-Project

Example Tutorial

Begin by opening up the example.exe binary. Then open cheat engine and attach to the process.
![alt text](https://i.imgur.com/GXpyUOp.png)

When looking at the console we can see a string. This can be used to figure out which is the location in memory that we are interested in.
![alt text](https://i.imgur.com/oYV4qLd.png)

Open cheat engines memory viewer and go to view->referenced strings. From here we can find the string that we saw. Click the string and double click the address on the right. This is a cross reference to the string.
![alt text](https://i.imgur.com/bo2PAVa.png)

Right here is where the first part of the key buffer is loaded into a register. We can take the address that is moved into the xmm register and put it into the byte viewer below to keep track of. (Right click the addresses of the left pane and click goto address)
![alt text](https://i.imgur.com/9j5at2k.png)

Below we can see the first part of the key decryption. It xors against our buffer. 
![alt text](https://i.imgur.com/4OE5wsK.png)

Open the debugger application and attach to our example. Right click and click go to address on the pxor instruction. From here copy the address and put it in to the debugger textbox. Click swbp for a software breakpoint.
![alt text](https://i.imgur.com/G7BWkVw.png)

You should see the instruction become an int3.
![alt text](https://i.imgur.com/nSccfZB.png)

We can then click enter in the console to continue executing the instructions.
![alt text](https://i.imgur.com/T4UeEW8.png)

Our debugger will stop once the breakpoint is hit. We then want to click step one time so that the pxor instruction is ran. After clicking step one more time so that the unencrypted value is moved back into the key buffer, we can see below in the byteview that a portion of the key is now decrypted.
![alt text](https://i.imgur.com/UkhvCZA.png)

Continue to step the debugger until we see another byte get decrypted. Those well versed in asm may see that we are executing a loop that xors a byte in the key buffer, advances the pointer we have to the buffer, then loops again decrypting the next bytes.
![alt text](https://i.imgur.com/PcQzE3L.png)

We can continue to step until the full key is decrypted or simple press continue to let the app proceed.
![alt text](https://i.imgur.com/SKEozBc.png)

Make sure to press continue if you decided to single step so that we can enter our key.

You can now copy the key from the memory view and input it into the console.
![alt text](https://i.imgur.com/zfGBrsO.png)




