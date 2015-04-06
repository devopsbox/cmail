<?php


class Auth {

	private static $auth = NULL;
	private $db;
	private $output;
	private $authorized;
	private $rights;
	private $user;
	private $delegates_user = NULL;
	private $delegates_address = NULL;

	private function __construct($db, $output) {

		$this->db = $db;
		$this->output = $output;
		$this->authorized = false;
		$this->rights = array();
	}

	/**
	 * get auth instance
	 */
	public static function getInstance($db, $output) {

		if (is_null(self::$auth)) {
			self::$auth = new self($db, $output);
		}
		return self::$auth;
	}

	public function getUser() {
		return $this->user;
	}

	public function hasRight($right) {

		if (isset($this->rights[$right]) && $this->rights[$right]) {
			return true;
		}

		return false;
	}

	public function isAuthorized() {
		return $this->authorized;
	}

	public function getDelegateUsers() {

		if (!is_null($this->delegates_user)) {
			return $this->delegates_user;
		}

		if (!$this->isAuthorized()) {
			return array();
		}
		
		$sql = "SELECT api_delegate FROM api_user_delegates WHERE api_user = :api_user";

		$params = array(
			":api_user" => $this->user
		);

		$delegates = $this->db->query($sql, $params, DB::F_ARRAY);

		$output = array();
		foreach($delegates as $delegate) {
			$output[] = $delegate["api_delegate"];
		}
		$this->delegates_user = $output;
		return $output;

	}
	public function getDelegateAddresses() {

		if (!is_null($this->delegates_addresses)) {
			return $this->delegates_addresses;
		}

		if (!$this->isAuthorized()) {
			return array();
		}
		
		$sql = "SELECT api_expression FROM api_address_delegates WHERE api_user = :api_user";

		$params = array(
			":api_user" => $this->user
		);

		$delegates = $this->db->query($sql, $params, DB::F_ARRAY);

		$output = array();
		foreach($delegates as $delegate) {
			$output[] = $delegate["api_expression"];
		}
		$this->delegates_addresses = $output;
		return $output;

	}

	private function getRights() {

		$sql = "SELECT api_right FROM api_access WHERE api_user = :api_user";

		$params = array(
			":api_user" => $this->user
		);

		$out = $this->db->query($sql, $params, DB::F_ARRAY);

		foreach($out as $right) {
			$this->rights[$right["api_right"]] = true;
		}

		$this->output->addDebugMessage("rights", $this->rights);
	}

	/**
	 * authorization function
	 * @param db database connection
	 * @param output output object
	 * @param auth object with authorization data (like user, pw)
	 * @return true if ok, else false
	 */
	function auth($auth) {
		$auth = array();
		if (!isset($_SERVER["PHP_AUTH_USER"]) || empty($_SERVER["PHP_AUTH_USER"])) {
			return false;
		}

		if (!isset($_SERVER["PHP_AUTH_PW"]) || empty($_SERVER["PHP_AUTH_PW"])) {
			return false;
		}

		$auth["user_name"] = $_SERVER["PHP_AUTH_USER"];
		$auth["password"] = $_SERVER["PHP_AUTH_PW"];

		if (!isset($auth["user_name"]) || empty($auth["user_name"]) ) {
			$this->output->add("status", "No auth username set.");
			return false;
		}

		$sql = "SELECT user_authdata FROM users WHERE user_name = :user_name";

		$params = array(
			":user_name" => $auth["user_name"]
		);

		$user = $this->db->query($sql, $params, DB::F_ARRAY);


		if (count($user) < 1) {
			$this->output->add("status", "Username not found.");
			return false;	
		}

		$user = $user[0];
		$this->user = $auth["user_name"];

		if (!isset($user["user_authdata"])) {
			$this->output->add("status", "User is not allowed to login.");
			return false;
		}

		$password = explode(":", $user["user_authdata"]);

		$pass = hash("sha256", $password[0] . $auth["password"]);

		$this->authorized = ($password[1] === $pass);

		if ($this->authorized) {
			$this->getRights();
		}

		return $this->authorized;
	}

}

?>
