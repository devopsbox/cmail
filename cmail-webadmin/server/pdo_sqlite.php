<?php

require_once("output.php");

/**
 * db wrapper class
 */
class DB {

	const F_SINGLE_OBJECT = 2;
	const F_OBJECT = 0;
	const F_ARRAY = 1;

	private $db;
	private $order = "";
	private $output;

	public function __construct($output) {
		$this->output = $output;
	}

	/**
	 * connect to database
	 */
	function connect() {
		global $dbpath;

		try {
			$this->db = new PDO('sqlite:' . $dbpath);
			$this->db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
		} catch (PDOException $e) {

			header("Status: 500 " . $e->getMessage());
			echo $e->getMessage();
			die();
		}

		return true;
	}


	/**
	 * set Order of a tag
	 */
	function setOrder($tag, $order) {
		$this->order = " ORDER BY " . $tag . " " . $order;
	}


	function prepare($sql) {
		$stm = $this->db->prepare($sql);

		if ($this->db->errorCode() > 0) {
			$this->output->addDebugMessage("db", $this->db->errorInfo());
			return null;
		}
		return $stm;
	}

	/**
	 * executes a prepared statement
	 */
	function execute($stm, $params) {
		$stm->execute($params);
		if ($this->db->errorCode() > 0) {
			$this->output->addDebugMessage("db", $this->db->errorInfo());
			error_log($this->db->errorInfo());
			return null;
		}
		return $stm;
	}

	/**
	 * queries an sql statement with the given params
	 */
	function query($sql, $params, $fetch) {
		global $orderBy;

		if (strpos($sql, "SELECT") !== false) {

			$sql .= $this->order;

			if (isset($_GET["limit"]) && !empty($_GET["limit"])) {
				$sql .= " LIMIT :limit";
				$params[":limit"] = $_GET["limit"];
			}
		}

		$stm = $this->prepare($sql);

		if (is_null($stm)) {
			return null;
		}

		$stm->execute($params);

		if (is_null($stm)) {
			return null;
		}

		switch ($fetch) {
			case DB::F_SINGLE_OBJECT:
				$output = $stm->fetch(PDO::FETCH_OBJ);
				break;
			case DB::F_OBJECT:
		       		$output = $stm->fetchAll(PDO::FETCH_OBJ);
				break;
			case DB::F_ARRAY:
			default:
				$output = $stm->fetchAll(PDO::FETCH_ASSOC);
				break;
		}
		$stm->closeCursor();

		return $output;
	}


	/**
	 * insert objects into database
	 * @param sql sql insert statement
	 * @param params list of input data.
	 */
	function insert($sql, $params) {
		global $orderBy;

		$stm = $this->db->prepare($sql);

		if ($this->db->errorCode() > 0) {
			$this->output->addDebugMessage("db", $this->db->errorInfo());
			return null;
		}

		foreach ($params as $param) {

			$stm->execute($param);

			if ($stm->errorInfo()[1] != 0) {
				$this->output->add("status", $stm->errorInfo()[2]);
				return null;
			}
		}

		$stm->closeCursor();

		return $this->lastInsertID();
	}


	/**
	 * starts a transaction
	 */
	function beginTransaction() {
		if (!$this->db->beginTransaction()) {
			$this->output->addDebugMessage("transaction", $this->db->errorInfo());
			return false;
		}
		return true;
	}


	/**
	 * commits a transaction
	 */
	function commit() {
		if (!$this->db->commit()) {
			$this->output->addDebugMessage("commit", $this->db->errorInfo());
			return false;
		}
		return true;
	}

	/**
	 * rollback a transaction
	 */
	function rollback() {
		$this->db->rollback();
	}

	/**
	 * returns the last inserted id
	 */
	function lastInsertID() {
		return $this->db->lastInsertId();
	}

}
