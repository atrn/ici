/*
 * ICI resource file
 */
auto ui = [array

    [struct
	widget = "app.label",
	font   = "-adobe-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*",
	pack   = "-padx 5 -side top -expand yes",
    ],

    [struct
	widget = "app.label",
	font   = "-adobe-helvetica-bold-r-*-*-18-*-*-*-*-*-*-*",
	text   = "A demonstration of the ICI Application Framework",
	pack   = "-padx 5 -side top -expand yes",
    ],

    [struct
	widget = "app.label",
	relief = "sunken",
	font   = "-adobe-helvetica-medium-r-*-*-16-*-*-*-*-*-*-*",
	text   =
	    "This program demonstrates extending the existing UI "
	    "objects\nin the application framework. The program defines "
	    "a new type\nof UI object, the TextWindow, that is used "
	    "to show users the\ncontents of text files.\n\nThis version "
	    "of the program is tagged \"extreme\" as the program\n is one "
	    "line.\n",
	justify = "left",
	pack    = "-padx 5 -pady 15 -side top -expand yes",
    ],

    [struct
	widget = "app.button",
	text   = "Load",
	action = "loadButton();",
	pack   = "-side left -fill both -expand yes",
    ],

    [struct
	widget = "app.button",
	text   = "Quit",
	action = "exit();",
	pack   = "-side left -fill both -expand yes",
    ],

];
