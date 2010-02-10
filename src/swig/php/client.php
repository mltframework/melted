<?php
/**
 * Client class
 *
 * This class will handle all communications with a server
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
class Client
{
	/**
	 * Address of the server
	 *
	 * @access	public
	 * @var		String
	 */
	var $address;
	
	/**
	 * Port to connect to on server
	 *
	 * @access	public
	 * @var		int
	 */
	var $port;
	
	/**
	 * Connection to the server
	 *
	 * @access	public
	 * @var		Resource
	 */
	var $connection;
	
	/**
	 * Tells if client is connected
	 *
	 * @access	public
	 * @var		boolean
	 */
	var $connected;
	
	/**
	 * Expect Welcome flag
	 *
	 * @access	public
	 * @var		boolean
	 */
	var $expectWelcome;
	
	/**
	 * Client Constructor
	 */
	function __construct($address, $port, $mode = 1) {
		$this->setup($address, $port, $mode);
		$this->_initialize();
	}
	
	/**
	 * Client Destructor
	 */
	function __destruct() {
		
	}
	
	/**
	 * Sets the address and port of the server
	 *
	 * @access	public
	 * @return	void
	 */
	function setup($address, $port, $mode = 1) {
		$this->address = $address;
		$this->port = $port;
		
		$this->connected = false;
		$this->expectWelcome = false;
		if( $mode > 1 ) {
			$mode = 1;
		}
		$this->mode = $mode;
	}
	
	/**
	 * Initialization function to be extended by descendent classes
	 *
	 * @access    protected
	 * @return    void
	 */
	function _initialize() {
		
	}
	
	/**
	 * Connects to the server using $address and $port
	 *
	 * @access	public
	 * @param	$expect_welcome - should we expect a welcome message
	 * @param	$welcome - welcome message
	 * @return	boolean - true if connected, false if not connected
	 */
	function connect($expectWelcome = false, &$welcome = null) {
		if( $this->connected ) {
			$this->disconnect();
		}
		$this->connected = false;
		$this->connection = @fsockopen($this->address, $this->port, $errno, $errstr, 30); 
		$this->expectWelcome = $expectWelcome;
		if( !$this->connection === false ) {
			$this->setBlockingMode($this->mode);
			$this->connected = true;
			if( $expectWelcome === true ) {
				$welcome = $this->read();
			}
		}
		return $this->connected;
	}
	
	/**
	 * Disconnects from the server
	 *
	 * @access	public
	 */
	function disconnect() {
		if( is_resource($this->connection) ) {
			fclose($this->connection);
		}
		$this->connected = false;
		return !$this->connected;
	}
	
	/**
	 * Attempts to reconnect to the server
	 *
	 * @access	public
	 */
	function reconnect() {
		$this->disconnect();
		return $this->connect($this->expectWelcome);
	}
	
	/**
	 * Writes a message to the server
	 *
	 * @access	public
	 * @param	String - Message to write to the server
	 * @return	number of bytes written or false on error
	 */
	function write($message) {
		if( $this->connected ) {
			if( !is_string($message) ) {
				return false;
			}
			$message = trim($message).PHP_EOL;
			$result = @fputs($this->connection, $message, strlen($message));
			if( $result == false ) {
				if( $this->reconnect() ) {
					return $this->write($message);
				} else {
					return $this->connected = false;
				}
			}
		} else {
			return false;
		}
		
		return $result;
	}
	
	/**
	 * Reads a message from the server
	 *
	 * @access	public
	 * @return	Response from server, or false on error
	 */
	function read() {
		$response = '';
		if( $this->connected ) {
			if( $this->mode ) {
				$response = @fgets($this->connection, 65535);
				if($response === false) {
					if( $this->reconnect() ) {
						return $this->read();
					} else {
						return $this->connected = false;
					}
				}
			} else {
				while($tmp = @fgets($this->connection)) {
					$response .= $tmp;
				}
			}
		} else {
			$response = '*** NOT CONNECTED ***';
		}
		
		return trim($response);
	}
	
	/**
	 * Sets the blocking mode of the connection
	 *
	 * @access	public
	 * @param	$mode
	 * @return	boolean
	 */
	function setBlockingMode($mode) {
		if( !is_int($mode) ) {
			return false;
		}
		if( $mode == 0 ) {
			$this->mode = 0;
		} else {
			$this->mode = 1;
		}
		stream_set_blocking($this->connection, $this->mode);
	}
	
	/**
	 * Returns the connection status of the server
	 *
	 * @access	public
	 * @return	boolean
	 */
	function isConnected() {
		return $this->connected;
	}
}

?>