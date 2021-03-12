# ttab
---
2021-03-11, version 0.93

### Usage
`ttab [OPTION]`

### Requirements
You'll need a C compiler, unless you somehow get this as a binary.  If that
is the case, then you should be A-Okay.

### Installation
Installation is not at all necessary for this program to function properly.
It's very basic.  If you wish to do so, however, after compilation (as
root):  `make install`

### Uninstallation
If you've installed via the 'make' option, then the easiest way to remove it
is to return to the directory from which you installed it to begin with (or
wherever else you're hiding the Makefile) and, as root, run `make uninstall`

### Description
This program acts as a basic calculator/tabulator for the terminal.  Just
type numbers and it will keep adding them to the 'register'.  You can clear
the register at any time, list a log of your activities and save what you've
done to a log file.

It can also take a list of numbers from stdin and add them all together.

### Command-line options
```
-h or --help
	Print some help text

-v or --version
	Print version and author info
```

### Commands during operation
```
c or clear
	Clear the register (reset to 0)

l or *
	Display the log

s or save [FILENAME]
	Save the log to a specified location.  If no location was provided
	initially, the program will automatically save to a file named
	ttab_YYYY-MM-DD_hh-mm-ss.log (timed and dated) in the present working
	directory.

s or /
	Quick-save the log to the present working directory.  If you've already
	saved to a log file, it will save to that log file again.

q or quit
	Exit program

h or help
	Display brief help text

u or undo
	Undo previous addition/subtraction

NUMBER..
	Repeat last addition/subtraction NUMBER times.  For example, if you've
	just added 5 to the tally, typing '4..' will then add a total of 20 to
	the tally (because 5 x 4 = 20).  Typing only '..' will repeat the last
	addition once, the same as typing '1..'
```

### Examples
```
ttab
	Start the program normally (with a captive interface)

ttab NUMBERS.txt
	Read output from NUMBERS.txt similar to the above example, printing one
	final result.  Note that only numbers will work (including positive or
	negative modifiers in front of the number... a + or a -)

cat NUMBERS.txt | ttab -
	Same as above, except piped in.

echo 1 2 3 4 5 | ttab -
	Sum the numbers (in this case, 1, 2, 3, 4 and 5) and print result.
```

###	Contact info, etc.
License:  [GPL3](https://www.gnu.org/licenses/gpl-3.0.en.html)

Author:  James Hendrie (hendrie dot james at gmail dot com)
