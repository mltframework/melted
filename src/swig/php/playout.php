<?php
/**
 * Playout Client
 *
 * The classes in this file are responsible for controlling the MVCP server.
 * 
 * The class Melted is responsible for handling the connection to the MVCP server as well
 * as passing commands and receiving responses from the server.
 *
 * The class Playout is responsible for retreiving and loading the appropriate items on the
 * schedule.  It will use the Melted class to initialize and update the server.
 *
 * PHP version 5.2.0+
 *
 * Licensed under The MIT License
 * Redistributions of files must retain the above copyright notice.
 *
 * @filesource
 * @copyright       2010 Skyler Sully
 * @link            %link%
 * @since           %since%
 * @version         $Revision: $
 * @modifiedby      $LastChangedBy: ssully$
 * @lastmodified    $Date: $
 * @license         http://www.opensource.org/licenses/mit-license.php The MIT License
 */
/**
 * Playout Controller.
 *
 * Retreives schedule information and performs functions to playout assets based on
 * the schedule on time.  Uses the Melted class
 *
 * @package       playout
 * @subpackage    application.vendors.socket.library.client
 */
class Playout
{
	/**
	 * Name of the module
	 *
	 * @access    public
	 * @var       string
	 */
	var $name = 'playout';
	
	/**
	 * Contains defaults for the Playout class.  They will be overwritten
	 * by the config passed to the constructor
	 *
	 * @access    protected
	 * @var       array
	 */
	var $_defaults = array(
		'host' => 'localhost',
		'port' => 5250,
		'mode' => 1,
	);
	
	/**
	 * Contains the settings necessary for the Playout Client.
	 *
	 * @access    public
	 * @var       array
	 */
	var $settings = array();
	
	/**
	 * SDI Cards
	 *
	 * @access    public
	 * @var       array
	 */
	var $units = array(
		'U0' => 0,
		'U1' => 1,
		'U2' => 2,
		'U3' => 3
	);
	
	/**
	 * Array of command responses from the MVCP server.
	 *
	 * @access    protected
	 * @var       array
	 */
	var $responses = array();
	
	/**
	 * Contains the MVCP client
	 *
	 * @access    public
	 * @var       object
	 */
	var $Mvcp;
	
	/**
	 * Contains the playlist for the current hour
	 *
	 * @access    public
	 * @var       array
	 */
	var $playlist = array();
	
	/**
	 * Playout constructor
	 */
	function __construct($config = array()) {
		$this->settings = array_merge($this->_defaults, $config);
		$this->_initialize();
	}
	
	/**
	 * Initializes the MVCP client
	 *
	 * @access    protected
	 * @return    void
	 */
	function _initialize() {
		extract($this->settings);
		$this->Mvcp = new MvcpClient($host, $port, $mode);
		$this->_connect();
	}
	
	/**
	 * Connects to the MVCP server.  If the connection has failed because the service is down,
	 * it attempts to restart the MVCP service.  If the service cannot be restarted after the
	 * $maxRestartTries setting, it will reboot the server.
	 *
	 * @access    protected
	 * @return    boolean
	 */
	function connect() {
		if( !$this->Mvcp->open() ) {
			return false;
		} else {
			return $this->Mvcp->isConnected();
		}
	}
	
	/**
	 * Disconnects from the MVCP server
	 *
	 * @access    protected
	 * @return    void
	 */
	function disconnect() {
		$this->Mvcp->close();
	}
	
	/**
	 * Shuts down the Playout service by stopping whatever is playing and clearing the unit.
	 *
	 * @access    public
	 * @return    void
	 */
	function shutdown() {
		if( !$this->Mvcp->isConnected() ) {
			$this->_connect();
		}
		$this->_command('stop');
		$this->_command('clear');
		$this->_disconnect();
	}
	
	/**
	 * Sets the key and value on the unit
	 *
	 * @access    public
	 * @param     $key
	 * @param     $value
	 * @param     $unit (optional)
	 * @return    void
	 */
	function uset($key, $value, $unit = 0) {
		$this->command('uset', array($key, $value), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Get the value for the specified key on the unit
	 *
	 * @access    public
	 * @param     $key
	 * @param     $unit (optional)
	 * @return    boolean
	 */
	function uget($key, $unit = 0) {
		$this->command('uget', array($key), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Loads a clip onto the unit
	 *
	 * @access    public
	 * @param     $clip
	 * @param     $in (optional)
	 * @param     $out (optional)
	 * @param     $unit (optional)
	 * @return    boolean
	 */
	function load($clip, $in = null, $out = null, $unit = 0) {
		$this->command('load', array($clip, $in, $out), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Appends a clip to the unit
	 *
	 * @access    public
	 * @param     $clip
	 * @param     $in (optional)
	 * @param     $out (optional)
	 * @param     $unit (optional)
	 * @return    boolean
	 */
	function append($clip, $in = null, $out = null, $unit = 0) {
		$this->command('apnd', array($clip, $in, $out), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Sets the in point for the current clip
	 *
	 * @access     public
	 * @param      $frame
	 * @param      $unit (optional)
	 * @return     boolean
	 */
	function set_in($frame, $unit = 0) {
		$this->command('set_in', array($frame), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Sets the out point for the current clip
	 *
	 * @access     public
	 * @param      $frame
	 * @param      $unit (optional)
	 * @return     boolean
	 */
	function set_out($frame, $unit = 0) {
		$this->command('set_out', array($frame), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Steps a certain number of frames (positive or negative)
	 *
	 * @access     public
	 * @param      $frames
	 * @param      $unit (optional)
	 * @return     boolean
	 */
	function step($frames, $unit = 0) {
		$this->command('step', array($frames), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Plays the current clip(s) on the specified unit
	 *
	 * @access    public
	 * @param     $unit (optional)
	 * @return    boolean
	 */
	function play($unit = 0) {
		$this->command('play', array(), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Stops the current playing clips on the specified unit
	 *
	 * @access    public
	 * @param     $unit (optional)
	 * @return    boolean
	 */
	function stop($unit = 0) {
		$this->command('stop', array(), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Clears the entire unit
	 *
	 * @access     public
	 * @param      $unit (optional)
	 * @return     boolean
	 */
	function clear($unit = 0) {
		$this->command('clear', array(), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Clears everything from the playlist that has already been played
	 *
	 * @access     public
	 * @param      $unit (optional)
	 * @return     boolean
	 */
	function wipe($unit = 0) {
		$this->command('wipe', array(), $errors, $unit);
		return $errors;
	}
	
	/**
	 * Convenience function to send commands to the MVCP server via the Mvcp class.
	 *
	 * @access    protected
	 * @param     $command - function name that corresponds with the command
	 *            in the Mvcp class
	 * @param     $arguments - array of arguments for command
	 * @return    boolean - status of command
	 */
	function command($command, $arguments = array(), &$errors = array(), $unit = 0) {
		$response = null;
		$this->Mvcp->setUnit($unit);
		$errors[] = $this->Mvcp->sendCommand($command, $arguments, $response);
		usleep(100000);
		$this->responses[] = $response;
	}
}

/**
 * Melted Client.
 *
 * Handles communication between the application and the MVCP server.
 *
 * @package       playout
 */
class MvcpClient extends Client
{
	/**
	 * Contains the name of the program
	 *
	 * @access    public
	 * @var       string
	 */
	var $name = 'melted';
	
	/**
	 * Contains all of the units that can be used in
	 * the MVCP server
	 *
	 * @access    public
	 * @var       array
	 */
	var $units = array('U0', 'U1', 'U2', 'U3');
	
	/**
	 * The current unit that operations are being done on
	 *
	 * @access    public
	 * @var       string
	 */
	var $_unit;
	
	/**
	 * Initializes the MVCP by setting the default unit to U0
	 *
	 * @access    protected
	 * @return    void
	 */
	function _initialize() {
		$this->setUnit();
	}
	
	/**
	 * Opens a connection with the MVCP server
	 *
	 * @access    public
	 * @return    boolean
	 */
	function open() {
		if( $this->isConnected() ) {
			return true;
		}
		$retval = $this->connect(true, $status);
		sleep(1);
		return $retval;
	}
	
	/**
	 * Closes the connection with the MVCP server
	 *
	 * @access    public
	 * @return    boolean
	 */
	function close() {
		if( !$this->isConnected() ) {
			return true;
		}
		$this->write('bye');
		return !$this->disconnect();
	}
	
	/**
	 * Restarts the MVCP service.  This should be done if a connection cannot be made
	 * with the Mvcp Client class
	 *
	 * @access    public
	 * @return    boolean
	 */
	function restart() {
		$output = array();
		$return = null;
		exec("/sbin/service {$this->name} restart 2&>1", $output, $return);
		if( $return != 0 ) {
			return false;
		} else {
			return true;
		}
	}
	
	/**
	 * Sets the current unit.  By default, sets the unit to the default
	 * unit U0
	 *
	 * @access    public
	 * @param     $unit
	 * @return    boolean
	 */
	function setUnit($unit = 0) {
		if( !is_int($unit) ) {
			$this->_unit = $this->units[0];
		} elseif( $unit < 0 || $unit > sizeof($this->units) - 1 ) {
			return false;
		}
		$this->_unit = $this->units[$unit];
		return true;
	}
	
	/**
	 * Sends the appropriate command to the MVCP server.
	 *
	 * @access    public
	 * @param     $command - command to run
	 * @param     $arguments - array of arguments
	 * @param     $response - response to return
	 * @return    boolean - status of command
	 */
	function sendCommand($command, $arguments = array(), &$response) {
		switch($command) {
			case 'uset':
				list($key, $value) = $arguments;
				$_command = "uset {$this->_unit} {$key}={$value}";
				break;
			case 'uget':
				list($key) = arguments;
				$_command = "uget {$this->_unit} {$key}";
				break;
			case 'load':
				list($clip, $in, $out) = $arguments;
				$_command = trim("load {$this->_unit} {$clip} {$in} {$out}");
				break;
			case 'apnd':
			case 'append':
				list($clip, $in, $out) = $arguments;
				$_command = trim("apnd {$this->_unit} {$clip} {$in} {$out}");
				break;
			case 'set_in':
			case 'setIn':
				list($frame) = $arguments;
				$_command = "sin {$this->_unit} {$frame}";
				break;
			case 'set_out':
			case 'setOut':
				list($frame) = $arguments;
				$_command = "sout {$this->_unit} {$frame}";
				break;
			case 'play':
				$_command = "play {$this->_unit}";
				break;
			case 'stop':
				$_command = "stop {$this->_unit}";
				break;
			case 'step':
				list($frames) = $arguments;
				$_command = "step {$this->_unit} {$frames}";
				break;
			case 'wipe':
				$_command = "wipe {$this->_unit}";
				break;
			case 'clear':
				$_command = "clear {$this->_unit}";
				break;
			default:
				return false;
				break;
		}
		return $this->runCommand($_command, $response);
	}
	
	/**
	 * Gets the status of the unit
	 *
	 * @access    public
	 * @return    array
	 */
	function status() {
		if( !$this->isConnected() ) {
			return array();
		}
		$this->write("usta {$this->_unit}");
		sleep(1);
		
		$return = $this->read();
		$status = $this->read();
		
		list($unit, $mode, $currentClip, $currentFrame, $speed, $fps, $in, $out,
			 $length, $tailClip, $tailPosition, $tailIn, $tailOut, $tailLength,
			 $seekable, $playlistGenNumber, $clipIndex) = split(' ', $status);
		
		return compact(array('unit', 'mode', 'currentClip', 'currentFrame', 'speed', 'fps', 'in', 'out',
					   'length', 'tailClip', 'tailPosition', 'tailIn', 'tailOut', 'tailLength',
					   'seekable', 'playlistGenNumber', 'clipIndex'));
	}
	
	/**
	 * Lists the clips on the unit
	 *
	 * @access    public
	 * @return    array
	 */
	function listClips() {
		if( !$this->isConnected() ) {
			return array();
		}
		$this->setBlockingMode(0);
		$this->write("list {$this->_unit}");
		sleep(1);
		
		$response = $this->read();
		$_list = explode(PHP_EOL, $response);
		$response = array_shift($_list);
		$genNum = array_shift($_list);
		foreach( $_list as $clip ) {
			list($index, $file, $in, $out, $realLength, $estimatedLength) = explode(' ', $clip);
			$list[] = compact(array('index', 'file', 'in', 'out', 'realLength', 'estimatedLength'));
		}
		$this->setBlockingMode(1);
		return $list;
	}
	
	/**
	 * Writes a command to the miracle server, gets the reponse and returns the
	 * status of the command as well as the command itself and the response code.
	 *
	 * @access    public
	 * @param     $command
	 * @param     $response - stores the command and response code
	 * @return    boolean true on success, false on failure
	 */
	function runCommand($command, &$response) {
		if( !$this->isConnected() ) {
			$response = "command - \"{$command}\" : not connected";
			return false;
		}
		$this->write($command);
		usleep(500000);
		$_response = $this->read();
		if( !empty($_response) ) {
			$_codes = explode(" ", $_response);
			if( isset($_codes[1]) && $_codes[1] != 'OK' ) {
				$error = false;
			} else {
				$error = true;
			}
		} else {
			$_response = "not connected";
			$error = true;
		}
		$response = "command - \"{$command}\" : {$_response}";
		return $error;
	}
}

?>