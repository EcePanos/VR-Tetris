//----------------------------------------------------------------------------//
//    Learning OpenGL: Εισαγωγικά Μαθήματα OpenGL                             //
//                                                                            //
//    Πλατφόρμα ανάπτυξης παραδειγμάτων                                       //
//                                                                            //
//    Εργαστήριο 3ο                                                           //
//----------------------------------------------------------------------------//

#include <stdlib.h>	  // Just for some standard functions
#include <stdio.h>    // Just for some ASCII messages
#include "visuals.h"  // Header file for our OpenGL functions
//#include <glut.h>
/*******************************************************************************
Main Program code
******************************************************************************/

int main(int argc, char* argv[])
{
	/* libovr must be initialized before we create the OpenGL context */
	ovr_Initialize(0);

	// initialize GLUT library state
	glutInit(&argc, argv);

	// Set up the display using the GLUT functions to
	// get rid of the window setup details:
	// - Use true RGB colour mode ( and transparency )
	// - Enable double buffering for faster window update
	// - Allocate a Depth-Buffer in the system memory or
	//   in the video memory if 3D acceleration available
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	// Define the main window size and initial position
	// ( upper left corner, boundaries included )
	glutInitWindowSize(480, 480);
	glutInitWindowPosition(50, 50);

	// Create and label the main window
	glutCreateWindow("3D Tetris");

	// Configure various properties of the OpenGL rendering context
	setup();

	// Callbacks for the GL and GLUT events:
	// Set the rendering function
	glutDisplayFunc(render);
	// Set the resize function
	glutReshapeFunc(resize);

	// Set functions for keyboard
	glutKeyboardFunc(keyboardDown);
	//glutSpecialFunc(keyboardSpecialDown);
	glutKeyboardUpFunc(keyboardUp);

	// Set idle
	glutTimerFunc(100, idle, 0);
	//glutIdleFunc(idle);

	// Set functions for mouse
	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMotion);

	//Enter main event handling loop
	glutMainLoop();
	return 0;
}

