swhx.app : Screenweaver HX boot loader for OS-X (10.3+ Intel/PCC)
swhx.exe : Screenweaver HX boot loader for Windows

The Screenweaver boot loader wraps the Neko VM into a regular desktop application. On launch, the loader will look for a compiled Neko file called 'app.n' in the current working directory. 

Once located, the bootloader will run 'app.n' in its virtual machine. Make sure that any resources (for example .swf files) that you program uses are available in the working directory too. 