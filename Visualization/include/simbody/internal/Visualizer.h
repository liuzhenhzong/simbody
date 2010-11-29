#ifndef SimTK_SIMBODY_VISUALIZER_H_
#define SimTK_SIMBODY_VISUALIZER_H_

/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTK Simbody(tm)                         *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2010 Stanford University and the Authors.           *
 * Authors: Peter Eastman, Michael Sherman                                    *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

/** @file 
Declares the Visualizer class used for collecting Simbody simulation results 
for display and interaction through the VisualizationGUI. **/


#include "simbody/internal/common.h"

#include <utility> // for std::pair

namespace SimTK {

class MultibodySystem;
class DecorationGenerator;

/** Provide simple visualization of and interaction with a Simbody simulation, 
in either a pass-through mode where timing is controlled by the simulation, or
in a real time mode where simulation results are synchronized with the real
time clock. Frames are sent to the renderer at a regular interval that is 
selectable, with a default rate of 30 frames/second. There are three operating
modes for the Visualizer, selectable via setMode():

@par PassThrough
This is the default mode. It sends through to the renderer \e every frame that
is received from the simulation, slowing down the simulation if necessary so 
that the frames are presented at a selected frame rate. 
But note that the simulation time will not be synchronized to real time; 
because Simbody simulations generally proceed at a variable rate, the 
regularly-spaced output frames will represent different amounts of simulated 
time. If you want real time and simulation time synchronized, use the RealTime 
mode.

@par Sampling
In this mode the simulation always runs at full speed. We send frames for 
display at a maximum rate given by the frame rate setting. After a frame is 
sent, all subsequent frames received from
the simulation are ignored until the frame interval has passed; then the
next received frame is displayed. This allows the simulation to proceed at
the fastest rate possible but time will be irregular and not all frames
generated by the simulation will be shown.

@par RealTime
Synchronize frame times with the simulated time, slowing down the 
simulation if it is running ahead of real time, as modifed by the
time scale; see setRealTimeScale(). Frames are sent to the renderer at the
selected frame rate. Smoothness is maintained by buffering 
up frames before sending them; interactivity is maintained by keeping the
buffer length below human perception time (150-200ms). The presence and size
of the buffer is selectable; see setDesiredBufferLengthInSec().

**/
class SimTK_SIMBODY_EXPORT Visualizer {
public:
class FrameController; // defined below
class EventListener;   // defined in Visualizer_EventListener.h


/** Construct new Visualizer using default window title (the name of the 
current executable). **/
Visualizer(MultibodySystem& system);
/** Construct new Visualizer with a given window title. **/
Visualizer(MultibodySystem& system, const String& title);
/** Event listener and decoration generator objects are destroyed here. **/
~Visualizer();
    
/** These are the operating modes for the Visualizer, with PassThrough the 
default mode. See the documentation for the Visualizer class for more
information about the modes. **/
enum Mode {
    /** Send through to the renderer every frame that is received from the
    simulator. **/
    PassThrough = 1,
    /** Sample the results from the simulation at fixed real time intervals
    given by the frame rate. **/
    Sampling    = 2,
    /** Synchronize real frame display times with the simulated time. **/
    RealTime    = 3
};

/** Set the operating mode for the Visualizer. See \ref Visualizer::Mode for 
choices, and the discussion for the Visualizer class for meanings. **/
void setMode(Mode mode);
/** Get the current mode being used by the Visualizer. See \ref Visualizer::Mode
for the choices, and the discussion for the Visualizer class for meanings. **/
Mode getMode() const;

/** Set the frame rate in frames/sec (of real time) that you want the 
Visualizer to attempt to achieve. This affects all modes. The default is 30 
frames per second for RealTime and Sampling modes; Infinity (that is, as fast 
as possible) for PassThrough mode. Set the frame rate to zero to return 
to the default behavior. **/
void setDesiredFrameRate(Real framesPerSec);
/** Get the current value of the frame rate the Visualizer has been asked to 
attempt; this is not necessarily the rate actually achieved. A return value of 
zero means the Visualizer is using its default frame rate, which may be
dependent on the current operating mode. 
@see setDesiredFrameRate() for more information. **/
Real getDesiredFrameRate() const;

/** In RealTime mode we normally assume that one unit of simulated time should
map to one second of real time; however, in some cases the time units are not 
seconds, and in others you may want to run at some multiple or fraction of 
real time. Here you can say how much simulated time should equal one second of
real time. For example, if your simulation runs in seconds, but you want to 
run twice as fast as real time, then call setRealTimeScale(2.0), meaning that 
two simulated seconds will pass for every one real second. This call will have 
no immediate effect if you are not in RealTime mode, but the value will be 
remembered.

@param[in]      simTimePerRealSecond
The number of units of simulation time that should be displayed in one second
of real time. Zero or negative value will be interpeted as the default ratio 
of 1:1. **/
void setRealTimeScale(Real simTimePerRealSecond);
/** Return the current time scale, which will be 1 by default.
@see setRealTimeScale() for more information. **/
Real getRealTimeScale() const;

/** When running an interactive realtime simulation, you can smooth out changes
in simulation run rate by buffering frames before sending them on for 
rendering. The length of the buffer introduces an intentional response time 
lag from when a user reacts to when he can see a response from the simulator. 
Under most circumstances a lag of 150-200ms is undetectable. The default 
buffer length is the time represented by the number of whole frames 
that comes closest to 150ms; 9 frames at 60fps, 5 at 30fps, 4 at 24fps, etc. 
To avoid frequent block/unblocking of the simulation thread, the buffer is
not kept completely full; you can use dumpStats() if you want to see how the
buffer was used during a simulation. Shorten the buffer to improve 
responsiveness at the possible expense of smoothness. Note that the total lag 
time includes not only the buffer length here, but also lag induced by the 
time stepper taking steps that are larger than the frame times. For maximum 
responsiveness you should keep the integrator step sizes limited to about 
100ms, or reduce the buffer length so that worst-case lag doesn't go much over
200ms. 
@param[in]      bufferLengthInSec
This is the target time length for the buffer. The actual length is the nearest
integer number of frames whose frame times add up closest to the request. If
you ask for a non-zero value, you will always get at least one frame in the
buffer. If you ask for zero, you'll get no buffering at all. To restore the
buffer length to its default value, pass in a negative number. **/
void setDesiredBufferLengthInSec(Real bufferLengthInSec);
/** Get the current value of the desired buffer time length the Visualizer 
has been asked to use for smoothing the frame rate, or the default value
if none has been requested. The actual value will differ from this number
because the buffer must contain an integer number of frames. 
@see getActualBufferTime() to see the frame-rounded buffer length **/
Real getDesiredBufferLengthInSec() const;
/** Get the actual length of the real time frame buffer in seconds, which
may differ from the requested time because the buffer contains an integer
number of frames. **/
Real getActualBufferLengthInSec() const;
/** Get the actual length of the real time frame buffer in number of frames. **/
int getActualBufferLengthInFrames() const;

/** Report that a new simulation frame is available for rendering. Depending
on the current Visualizer::Mode, handling of the frame will vary:

@par PassThrough
All frames will be rendered, but the calling thread (that is, the simulation) 
may be blocked if the next frame time has not yet been reached or if the 
renderer is unable to keep up with the rate at which frames are being supplied 
by the simulation.

@par Sampling 
The frame will be rendered immediately if the next sample time has been reached
or passed, otherwise the frame will be ignored and report() will return 
immediately.

@par RealTime
Frames are queued to smooth out the time stepper's variable time steps. The 
calling thread may be blocked if the buffer is full, or if the simulation time
is too far ahead of real time. Frames will be dropped if they come too 
frequently; only the ones whose simulated times are at or near a frame time 
will be rendered. Frames that come too late will be queued for rendering as 
soon as possible, and also reset the expected times for subsequent frames so 
that real time operation is restored. **/
void report(const State& state);

/** This method draws a frame unconditionally without queuing or checking
the frame rate. Typically you should use the report() method instead, and
let the the internal queuing system decide when to call drawFrameNow(). **/
void drawFrameNow(const State& state);

/** Add a new event listener to this Visualizer, methods of which will be
called when the GUI detects user-driven events like key presses, menu picks, 
or mouse moves. See \ref Visualizer::EventListener for more information. 
The Visualizer takes over ownership of the supplied \a listener object and 
deletes it upon destruction of the Visualizer. **/
void addEventListener(EventListener* listener);

/** Add a new frame controller to this Visualizer, methods of which will be
called just prior to rendering a frame for the purpose of simulation-controlled
camera positioning and other frame-specific effects. 
See \ref Visualizer::FrameController for more information. The Visualizer takes 
over ownership of the supplied \a controller object and deletes it destruction 
of the Visualizer. **/ 
void addFrameController(FrameController* controller);

/** @name SceneBuilding Scene-building methods
These methods are used to add permanent elements to the scene being displayed
by the Visualizer. Once added, these elements will contribute to every frame.
Calling one of these methods requires writable (non-const) access to the 
Visualizer object. Note that adding DecorationGenerators does allow different
geometry to be produced for each frame; however, once added a 
DecorationGenerator will be called for \e every frame generated. **/
/**@{**/

/** Set the position and orientation of the ground plane.
@param axis     the axis to which the ground plane is perpendicular
@param height   the position of the ground plane along the specified axis **/
void setGroundPosition(const CoordinateAxis& axis, Real height);

/** Add a new pull-down menu to the VisualizationGUI's display. The button
label is given in \a title, and a list of (string,int) pairs defines the menu 
and submenu items. The strings have a pathname-like syntax, like "submenu/item1",
"submenu/item2", "submenu/lowermenu/item1", etc. that is used to define the
pulldown menu layout. **/
void addMenu(const String& title, const Array_<std::pair<String, int> >& items);

/** Add an always-present, body-fixed piece of geometry like the one passed in,
but attached to the indicated body. The supplied transform is applied on top of
whatever transform is already contained in the supplied geometry, and any body 
index stored with the geometry is ignored. **/
void addDecoration(MobilizedBodyIndex, const Transform& X_BD, 
                   const DecorativeGeometry&);

/** Add an always-present rubber band line, modeled after the DecorativeLine 
supplied here. The end points of the supplied line are ignored, however: at 
run time the spatial locations of the two supplied stations are calculated and 
used as end points. **/
void addRubberBandLine(MobilizedBodyIndex b1, const Vec3& station1,
                        MobilizedBodyIndex b2, const Vec3& station2,
                        const DecorativeLine& line);

/** Add a DecorationGenerator that will be invoked to add dynamically generated
geometry to each frame of the the scene. The Visualizer assumes ownership of the 
object passed to this method, and will delete it when the Visualizer is 
deleted. **/
void addDecorationGenerator(DecorationGenerator* generator);
/**@}**/

/** @name FrameControl Methods for controlling how a frame is displayed
These methods can be called prior to rendering a frame to control how the 
camera is positioned for that frame. These can be invoked from within a
FrameControl object for runtime camera control. **/
/**@{**/

/** Set the transform defining the position and orientation of the camera. **/
void setCameraTransform(const Transform& transform) const;

/** Move the camera forward or backward so that all geometry in the scene is 
visible. **/
void zoomCameraToShowAllGeometry() const;

/** Rotate the camera so that it looks at a specified point.
@param point        the point to look at
@param upDirection  a direction which should point upward as seen by the camera
**/
void pointCameraAt(const Vec3& point, const Vec3& upDirection) const;

/** Set the camera's vertical field of view, measured in radians. **/
void setCameraFieldOfView(Real fov) const;

/** Set the distance from the camera to the near and far clipping planes. **/
void setCameraClippingPlanes(Real nearPlane, Real farPlane) const;

/// OBSOLETE NAME: don't use this; it will be removed in a later release.
void zoomCameraToIncludeAllGeometry() const {zoomCameraToShowAllGeometry();}
/**@}**/

/** @name VizDebugging Methods for debugging and statistics **/
/**@{**/
/** Dump statistics to the given ostream (e.g. std::cout). **/
void dumpStats(std::ostream& o) const;
/** Reset all statistics to zero. **/
void clearStats();
/**@}**/

/** @name VizInternal Internal use only **/
/**@{**/
const Array_<EventListener*>& getEventListeners() const;
const Array_<FrameController*>& getFrameControllers() const;
/**@}**/

class VisualizerRep;
//--------------------------------------------------------------------------
                                private:
VisualizerRep* rep;

const VisualizerRep& getRep() const {assert(rep); return *rep;}
VisualizerRep&       updRep() const {assert(rep); return *rep;}
};

/** This abstract class represents an object that will be invoked by the
Visualizer just prior to rendering each frame. You can use this to call any
of the const (runtime) methods of the Visualizer, typically to control the 
camera, and you can also add some geometry to the scene, print messages to 
the console, and so on. **/
class SimTK_SIMBODY_EXPORT Visualizer::FrameController {
public:
    /** The Visualizer is just about to generate and render a frame 
    corresponding to the given State. 
    @param[in]          viz     
        The Visualizer that is doing the rendering.
    @param[in]          state   
        The State that is being used to generate the frame about to be
        rendered by \a viz.
    @param[in,out]      geometry 
        DecorativeGeometry being accumulated for rendering in this frame;
        be sure to \e append if you have anything to add.
    **/
    virtual void generateControls(const Visualizer&           viz, 
                                  const State&                state,
                                  Array_<DecorativeGeometry>& geometry) = 0;

    /** Destructor is virtual; be sure to override it if you have something
    to clean up at the end. **/
    virtual ~FrameController() {}
};

} // namespace SimTK

#endif // SimTK_SIMBODY_VISUALIZER_H_
