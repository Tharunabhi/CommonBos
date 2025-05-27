#include <iostream>
#include <nlohmann/json.hpp>
#include "../channel/amqpPublisher.h"
#include "fccboscom.pb.h"
#include "cbosparser.h"

using json = nlohmann::json;
using namespace std;

// Define static members of cbosparser
Amqp::amqpPublisher cbosparser::publisher;
bool cbosparser::amqp_initialized = false;

// Forward declarations of helper functions
static string handleSetRoConfiguration(const json& parsed, int apiVersion);
static string handleGetFCCStatus(int apiVersion);

string cbosparser::cbosparse(const char* buffer) {
    if (!amqp_initialized) {
        // Use FCCBOS and FCCDATA for sending data TO BOS
        publisher.createAmqpChannel("localhost", 5672, "BOSFCC", "BOSDATA");
        amqp_initialized = true;
    }

    string data(buffer);

    // Locate STX(0x02) and ETX(0x03)
    size_t start = data.find(static_cast<char>(0x02));
    size_t end = data.find(static_cast<char>(0x03));
    if (start == string::npos || end == string::npos || start >= end) {
        return generateResponse("Unknown", 0, false);
    }

    string jsonString = data.substr(start + 1, end - start - 1);

    try {
        auto parsed = json::parse(jsonString);
        string apiName = parsed.value("Api_Name", "Unknown");
        int apiVersion = parsed.value("Api_Version", 0);

        if (apiName == "Set_RoConfiguration") {
            return handleSetRoConfiguration(parsed, apiVersion);
        } else if (apiName == "Get_FCCStatus") {
            return handleGetFCCStatus(apiVersion);
        } else {
            return generateResponse(apiName, apiVersion, false);
        }
    }
    catch (const json::parse_error& e) {
        cerr << "[ERROR] JSON parse error: " << e.what() << endl;
        return generateResponse("INVALID_JSON", 1, false);
    }
}

static string handleSetRoConfiguration(const json& parsed, int apiVersion) {
    auto dataObj = parsed["Data"];
    fccboscom::DbConf dbconf;
    dbconf.set_tablename("public.rodetails");
    dbconf.set_operation(1);  // Insert

    auto addField = [&](const std::string& key, const std::string& mappedKey, int dataType = 2) {
        if (dataObj.contains(key)) {
            fccboscom::DataModule* dm = dbconf.add_datamodule();
            dm->set_key(mappedKey);
            dm->set_value(dataObj.value(key, ""));
            dm->set_datatype(dataType);
        }
    };

    // Map fields from JSON to Protobuf DB columns
    addField("RO_Code", "rocode");
    addField("Station_Name", "roname");
    addField("Station_Type", "rotype");
    addField("Dealer_Name", "dealername");
    addField("Addr_line1", "adress");
    addField("City", "city");
    addField("Pincode", "pincode");
    addField("Contact", "dealerphonenumber");
    addField("Mobile_No", "dealermobilenumber");
    addField("Email_addr", "dealeremail");

    


    fccboscom::RoData rodata;
    rodata.set_msgid(1);
    fccboscom::DbConf* newConf = rodata.add_dbconf();
    *newConf = dbconf;

    string serialized;
    if (!rodata.SerializeToString(&serialized)) {
        cerr << "[ERROR] Failed to serialize Protobuf message\n";
        return cbosparser::generateResponse("Set_RoConfiguration", apiVersion, false);
    }

    cbosparser::publisher.sendDataToFcc(nullptr, serialized);
    cout << "[INFO] Protobuf Message Sent to RabbitMQ\n";
    return cbosparser::generateResponse("Set_RoConfiguration", apiVersion, true);
}

static string handleGetFCCStatus(int apiVersion) {
    json responseData = json::object();
    responseData["Api_Name"] = "Get_FCCStatus";
    responseData["Api_Version"] = apiVersion;
    responseData["Is_Valid"] = true;

    json dataObj = json::object();
    dataObj["Vendor_Name"] = "Fueltrans";
    dataObj["FCC_Version"] = "2";
    dataObj["Is_Master"] = true;
    dataObj["Is_ConfigOK"] = true;
    dataObj["Is_ServiceRun"] = true;
    dataObj["Is_Have_Control"] = 1;
    dataObj["Is_Have_Trx"] = false;
    dataObj["Is_Have_Del"] = false;

    responseData["Data"] = dataObj;

    char STX = 0x02;
    char ETX = 0x03;
    return string(1, STX) + responseData.dump() + string(1, ETX);
}

string cbosparser::generateResponse(const std::string& apiName, int version, bool isValid) {
    json response;
    response["Api_Name"] = apiName;
    response["Api_Version"] = version;
    response["Is_Valid"] = isValid;

    char STX = 0x02;
    char ETX = 0x03;
    return string(1, STX) + response.dump() + string(1, ETX);
}
