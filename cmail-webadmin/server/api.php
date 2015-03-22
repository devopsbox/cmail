<?php

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

include_once("config.php");
include_once("output.php");
include_once("pdo_sqlite.php");
include_once("user.php");
include_once("address.php");

// auth plugin
include_once("auth.php");

main();

function getApiEndPoint() {
	$api_points = array(
		"get_user",
		"get_users",
		"get_addresses",
		"get_address",
		"get_addresses_by_user",
		"add_user",
		"delete_user",
		"add_address",
		"set_password",
		"delete_address",
		"update_user",
		"update_address",
		"switch_addresses"
	);

	foreach ($api_points as $point) {
		if (isset($_GET[$point])) {
			return $point;
		}
	}
}

function main() {

	// init
	$output = Output::getInstance();
	$db = new DB($output);

	$user = new User($db, $output);
	$address = new Address($db, $output);

	// db connection
	if (!$db->connect()) {
		header("WWW-Authenticate: Basic realm=\"cmail API Access (Invalid Credentials for " . $_SERVER['PHP_AUTH_USER'] . ")\"");
		header("HTTP/1.0 401 Unauthorized");

		die();
	}

	$endPoint = getApiEndPoint();
	$http_raw = file_get_contents("php://input");
	$obj = json_decode($http_raw, true);

	$output->addDebugMessage("payload", $obj);


	if (!auth($db, $output, $obj["auth"])) {
		header("WWW-Authenticate: Basic realm=\"cmail Access (Invalid Credentials for " . $_SERVER['PHP_AUTH_USER'] . ")\"");
		header("HTTP/1.0 401 Unauthorized");

		$output->write();
		die();
	}

	switch ($endPoint) {
	case "get_users":
		$user->getAll();
		break;
	case "get_user":
		$user->get($obj["username"]);
		break;
	case "get_addresses":
		$address->getAll();
		break;
	case "get_address":
		$address->get($obj["address"]);
		break;
	case "get_addresses_by_user":
		$address->getByUser($obj["username"]);
		break;
	case "add_user":
		$user->add($obj["user"]);
		break;
	case "delete_user":
		$user->delete($obj["username"]);
		break;
	case "add_address":
		$address->add($obj["address"]);
		break;
	case "set_password":
		$user->set_password($obj["user"]);
		break;
	case "delete_address":
		$address->delete($obj["expression"]);
		break;
	case "update_user":
		$user->update($obj["user"]);
		break;
	case "update_address":
		$address->update($obj["address"]);
		break;
	case "switch_addresses":
		$address->switchOrder($obj["address1"], $obj["address2"]);
		break;
	default:
		$output->add("status", "nothing to do.");
		break;
	}

	$output->write();
}