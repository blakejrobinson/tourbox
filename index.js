const EventEmitter = require('events');
const path = require('path');

let tourboxAddon;
try {
  tourboxAddon = require('./build/Release/tourbox.node');
} catch (err) {
  try {
    tourboxAddon = require('./build/Debug/tourbox.node');
  } catch (err2) {
    throw new Error('TourBox native addon not found. Please run "npm install" to build the addon.');
  }
}

class TourBox extends EventEmitter {
  constructor() {
    super();
    this.isRunning = false;
    this.server = null;
    this.rawCallback = null;
  }

  /**
   * Start the TourBox server
   * @param {number} port - Port to listen on (default: 50500)
   * @param {string} ip - IP address to bind to (default: "127.0.0.1", use "0.0.0.0" for all interfaces)
   * @returns {boolean} Success status
   */
  startServer(port = 50500, ip = "127.0.0.1") {
    if (this.isRunning) {
      console.warn('TourBox server is already running');
      return false;
    }

    try {
      // Create server with event callback and optional raw callback
      this.server = tourboxAddon.createServer(port, 
        (eventName, data) => {
          // Handle connection events (data is connection info object)
          if (eventName === 'connect' || eventName === 'disconnect') {
            this.emit(eventName, data);
            this.emit('*', eventName, data);
          } else {
            // Handle control events (data is count)
            this.emit(eventName, data);
            this.emit('*', eventName, data);
          }
        },
        ip,
        this.rawCallback ? (buffer) => {
          this.rawCallback(buffer);
        } : undefined
      );

      if (this.server) {
        this.isRunning = true;
        //console.log(`TourBox server started on ${ip}:${port}`);
        return true;
      }
    } catch (error) {
      console.error('Failed to start TourBox server:', error.message);
    }
    
    return false;
  }

  /**
   * Stop the TourBox server
   * @returns {boolean} Success status
   */
  stopServer() {
    if (!this.isRunning || !this.server) {
      console.warn('TourBox server is not running');
      return false;
    }

    try {
      tourboxAddon.stopServer(this.server);
      this.server = null;
      this.isRunning = false;
      //console.log('TourBox server stopped');
      return true;
    } catch (error) {
      console.error('Failed to stop TourBox server:', error.message);
      return false;
    }
  }

  /**
   * Check if server is running
   * @returns {boolean} Running status
   */
  isServerRunning() {
    return this.isRunning;
  }

  /**
   * Set raw data callback to receive raw TourBox protocol data
   * @param {function} callback - Function that receives Buffer objects
   * @returns {TourBox} This instance for chaining
   */
  raw(callback) {
    if (typeof callback !== 'function') {
      throw new Error('Raw callback must be a function');
    }
    this.rawCallback = callback;
    return this;
  }

  /**
   * Get available control names
   * @returns {string[]} Array of control names
   */
  getAvailableControls() {
    return [
      'Knob CCW', 'Knob CW', 'Knob Press', 'Knob Release',
      'Scroll Down', 'Scroll Up', 'Scroll Press', 'Scroll Release',
      'Dial CCW', 'Dial CW', 'Dial Press', 'Dial Release',
      'Up Press', 'Up Release', 'Down Press', 'Down Release',
      'Left Press', 'Left Release', 'Right Press', 'Right Release',
      'Tall Press', 'Tall/Side Release', 'Side Press',
      'Short Press', 'Short Release', 'Top Press', 'Top Release',
      'Tour Press', 'Tour Release', 'C1 Press', 'C1 Release',
      'C2 Press', 'C2 Release'
    ];
  }

  /**
   * Get button held state.
   * Usage: buttonState(name) -> global check across servers
   *        buttonState(serverId, name) -> check specific server
   */
  buttonState(arg1, arg2) {
    try {
      if (arguments.length === 1 && typeof arg1 === 'string') {
        return !!tourboxAddon.buttonState(arg1);
      } else if (arguments.length === 2 && typeof arg1 === 'number' && typeof arg2 === 'string') {
        return !!tourboxAddon.buttonState(arg1, arg2);
      } else {
        throw new Error('buttonState expects (name) or (serverId, name)');
      }
    } catch (err) {
      console.error('buttonState error:', err && err.message ? err.message : err);
      return false;
    }
  }
}

// Create and export singleton instance
const tourbox = new TourBox();

// Graceful shutdown
process.on('SIGINT', () => {
  //console.log('\nShutting down TourBox server...');
  tourbox.stopServer();
  process.exit(0);
});

process.on('SIGTERM', () => {
  tourbox.stopServer();
  process.exit(0);
});

module.exports = tourbox;