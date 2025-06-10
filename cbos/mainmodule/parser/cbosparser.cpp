#include <iostream>
#include <nlohmann/json.hpp>
#include "../channel/amqpPublisher.h"
#include "../../Config/config.h"
#include "fccboscom.pb.h"
#include "cbosparser.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

using json = nlohmann::json;
using namespace std;

// Define static members of cbosparser

Amqp::amqpPublisher cbosparser::publisher;
bool cbosparser::amqp_initialized = false;

std::unordered_map<std::string, int> cbosparser::productNameToIdMap;
std::unordered_map<int, int> cbosparser::tankNoToIdMap;


struct DispenserInfo {
    std::string make;
    std::string model;
    std::string pversion;
    int firmwareVersion;
};

// look up file
std::unordered_map<int, DispenserInfo> dispenserMap = {
    {1,  {"Tatsuno", "Tatsuno", "_2_0", 3845}},
    {3,  {"Midco", "Midco", "_3_10", 2301}},
    {7,  {"Wayne", "Wayne", "_2_7", 3433}},
    {9,  {"Gilbarco", "GilbarcoPostMix", "_1_14", 2202}},
    {10, {"Gilbarco", "Gilbarco", "_1_14", 2202}},
    {13, {"Tokheim", "quantum", "_1_79", 2101}},
    {15, {"Midco", "Midco Cobra", "_3_10", 2301}},
    {16, {"Midco", "SFH", "_3_10", 2301}},
    {17, {"Midco", "Aquafill", "_3_16", 2304}},
    {19, {"Gilbarco", "Gilbarco Tulip I", "_1_14", 2202}},
    {20, {"Tokheim", "quantum", "_1_79", 2101}},
    {22, {"Midco", "Midco Bullet", "_3_10", 2301}},
    {23, {"Midco", "Midco MMS", "_3_10", 2301}},
    {25, {"Gilbarco", "Gilbarco Sprint", "_1_14", 2202}},
    {29, {"Gilbarco", "Tulip2", "_1_14", 2205}},
    {30, {"Tokheim", "Tokheim PostMix", "_1_79", 2101}},
    {31, {"Gilbarco", "Gilbarco Endavour", "_1_14", 2202}},
    {32, {"Tokheim", "Tokheim Kaizan", "_1_79", 2101}},
};

DispenserInfo getDispenserInfo(int dispenserId) {
    auto it = dispenserMap.find(dispenserId);
    if (it != dispenserMap.end()) {
        return it->second;
    } else {
        // Return a default unknown dispenser info if not found
        return {"Unknown", "Unknown", "_0_0", 0};
    }
}


std::string convertHexParaToDecimalString(const std::string& hex) {
    unsigned int decimalValue;
    std::stringstream ss;
    ss << std::hex << hex;
    ss >> decimalValue;
    return std::to_string(decimalValue);
}


// Gets current system time in "YYYY-MM-DD HH:MM:SS" format
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime);

    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%F %T", localTime); // "%F %T" = "YYYY-MM-DD HH:MM:SS"
    return std::string(buffer);
}


static void addFieldToDbConf(const json& obj, fccboscom::DbConf* dbconf, const string& sourceField, const string& targetField, int dataType = 2) {
    if (obj.contains(sourceField)) {
        fccboscom::DataModule* dm = dbconf->add_datamodule();
        dm->set_key(targetField);

        if (obj[sourceField].is_boolean()) {
            dm->set_value(obj[sourceField].get<bool>() ? "1" : "0");  // convert bool to "1"/"0"
        }
        else if (obj[sourceField].is_number_integer()) {
            dm->set_value(std::to_string(obj[sourceField].get<int>()));
        }
        else if (obj[sourceField].is_number_float()) {
            dm->set_value(std::to_string(obj[sourceField].get<double>()));
        }
        else {
            dm->set_value(obj[sourceField].get<std::string>());
        }

        dm->set_datatype(dataType);
    }
}

// setting hardcoded values to the db

static void addHardcodedFieldToDbConf(fccboscom::DbConf* dbconf, const string& targetField, const string& value, int dataType = 2) {
    fccboscom::DataModule* dm = dbconf->add_datamodule();
    dm->set_key(targetField);
    dm->set_value(value);
    dm->set_datatype(dataType);
}


// Utility: Serialize and send

static bool serializeAndSendToFcc(fccboscom::RoData& rodata, const string& apiName, int apiVersion) {
    string serialized;
    if (!rodata.SerializeToString(&serialized)) {
        cerr << "[ERROR] Failed to serialize message for " << apiName << endl;
        return false;
    }
    cbosparser::publisher.sendDataToFcc(nullptr, serialized);
    cout << "[INFO] " << apiName << " Message Sent\n";
    return true;
}

string cbosparser::cbosparse(const char* buffer) {

   //cout << "DATA PACKEt" << buffer << endl;

    if (!amqp_initialized) {
        publisher.createAmqpChannel("localhost", 5672, "BOSFCC", "BOSDATA");
        amqp_initialized = true;
    }

    string data(buffer);
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

        if (apiName == "Get_FCCStatus") {

            return handleGetFCCStatus(apiVersion);
        } 
        else if (apiName == "Set_RoConfiguration") {

            return handleSetRoConfiguration(parsed, apiVersion);

        } 
        
        else if (apiName == "Set_ProductConfiguration") {

            return handleSetProductConfiguration(parsed, apiVersion);

        } 
        
        else if (apiName == "Set_GradeConfiguration"){

            return handleSetGradeConfiguration(parsed, apiVersion);
        }

        else if (apiName == "Set_TankConfiguration") {

    string response = handleSetTankConfiguration(parsed, apiVersion);
    
    fccboscom::RoData consoleData;
    consoleData.set_msgid(1);
    appendAtgConsoleDbConf(&consoleData);
    serializeAndSendToFcc(consoleData, "ATG_Console_Insert", apiVersion);

    const auto& dataObj = parsed.value("Data", json::object());
    const auto& tankList = dataObj.value("Tank_List", json::array());
    appendAtgProbelistDbConfFromTankList(tankList, apiVersion);

    return response;
}


        else if(apiName == "Set_DispenserConfiguration"){

            return handleSetDispenserConfiguration(parsed, apiVersion);
        }

        else {

            return generateResponse(apiName, apiVersion, true);
        }

    } catch (const json::parse_error& e) {

        cerr << "[ERROR] JSON parse error: " << e.what() << endl;
        return generateResponse("INVALID_JSON", 1, false);
    }
}


string handleGetFCCStatus(int apiVersion) {
    json responseData = {
        {"Api_Name", "Get_FCCStatus"},
        {"Api_Version", apiVersion},
        {"Is_Valid", true},
        {"Data", {
            {"Vendor_Name", "Fueltrans"},
            {"FCC_Version", "2"},
            {"Is_Master", true},
            {"Is_ConfigOK", true},
            {"Is_ServiceRun", true},
            {"Is_Have_Control", 1},
            {"Is_Have_Trx", false},
            {"Is_Have_Del", false}
        }}
    };

    char STX = 0x02;
    char ETX = 0x03;
    return string(1, STX) + responseData.dump() + string(1, ETX);
}


void appendAtgConsoleDbConf(fccboscom::RoData* rodata) {
    fccboscom::DbConf atgDbconf;
    atgDbconf.set_tablename("public.atgconsole");
    atgDbconf.set_operation(1);  // 1 = INSERT

    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(now, "%Y-%m-%d %H:%M:%S");

    // Read config
    auto config = readConfigFile("Config/Config.properties");
    std::string comportid = config.count("atgcomportid") ? config["atgcomportid"] : "0"; // fallback

    addHardcodedFieldToDbConf(&atgDbconf, "id", "1", 1);
    addHardcodedFieldToDbConf(&atgDbconf, "uid", "1", 1);
    addHardcodedFieldToDbConf(&atgDbconf, "consolename", "BCT Console", 2);
    addHardcodedFieldToDbConf(&atgDbconf, "make", "BCTSoftConsole", 2);
    addHardcodedFieldToDbConf(&atgDbconf, "model", "TLS4", 2);
    addHardcodedFieldToDbConf(&atgDbconf, "pversion", "_1_0", 2);
    addHardcodedFieldToDbConf(&atgDbconf, "comportid", comportid, 1); // from config
    addHardcodedFieldToDbConf(&atgDbconf, "aiptimeout", "20", 1);
    addHardcodedFieldToDbConf(&atgDbconf, "isenable", "1", 1);
    addHardcodedFieldToDbConf(&atgDbconf, "updatetime", oss.str(), 2);

    *rodata->add_dbconf() = atgDbconf;
}

void appendAtgProbelistDbConfFromTankList(const json& TankList, int apiVersion) {
    bool allSuccess = true;

    // Get current time string once
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(now, "%Y-%m-%d %H:%M:%S");
    std::string currentTimeStr = oss.str();

    for (const auto& tank : TankList) {
        int T_No = tank.value("T_No", 0);
        auto it = cbosparser::tankNoToIdMap.find(T_No);
        if (it == cbosparser::tankNoToIdMap.end()) {
            std::cerr << "[Error] T_No " << T_No << " not found!\n";
            allSuccess = false;  // Mark as failure
            continue;
        }

        int tankId = it->second;
        std::cerr << "[ATG] T_No: " << T_No << " → tank.id: " << tankId << "\n";

        fccboscom::DbConf dbconf;
        dbconf.set_tablename("public.atgprobelist");
        dbconf.set_operation(1);

        addHardcodedFieldToDbConf(&dbconf, "id", std::to_string(tankId), 1);
        addHardcodedFieldToDbConf(&dbconf, "uid", std::to_string(tankId), 1);
        addHardcodedFieldToDbConf(&dbconf, "make", "Startitalina", 2);
        addHardcodedFieldToDbConf(&dbconf, "model", "st123", 2);
        addHardcodedFieldToDbConf(&dbconf, "atgtype", "L", 2);
        addHardcodedFieldToDbConf(&dbconf, "isenable", "1", 1);
        addHardcodedFieldToDbConf(&dbconf, "updatetime", currentTimeStr, 2);

        // Log before sending
        std::cerr << "[DEBUG] Inserting tank id: " << tankId << "\n";

        fccboscom::RoData rodata;
        rodata.set_msgid(1);
        *rodata.add_dbconf() = dbconf;

        if (!serializeAndSendToFcc(rodata, "Set_ATGProbelistConfiguration", apiVersion)) {
            std::cerr << "[Error] Failed sending config for tank id " << tankId << "\n";
            allSuccess = false;  // Mark as failure
        }
    }

    // You no longer return anything because function is now void
    std::cerr << "[INFO] ATG_Probelist_Insert " << (allSuccess ? "succeeded." : "had some errors.") << "\n";
}



string handleSetRoConfiguration(const json& parsed, int apiVersion) {
    const auto& dataObj = parsed.value("Data", json::object());
    fccboscom::DbConf dbconf;
    dbconf.set_tablename("public.rodetails");
    dbconf.set_operation(1);

    addFieldToDbConf(dataObj, &dbconf, "RO_Code", "rocode");
    addFieldToDbConf(dataObj, &dbconf, "Station_Name", "roname");
    addFieldToDbConf(dataObj, &dbconf, "Station_Type", "rotype");
    addFieldToDbConf(dataObj, &dbconf, "Dealer_Name", "dealername");
    addFieldToDbConf(dataObj, &dbconf, "Addr_line1", "adress");
    addFieldToDbConf(dataObj, &dbconf, "City", "city");
    addFieldToDbConf(dataObj, &dbconf, "Pincode", "pincode");
    addFieldToDbConf(dataObj, &dbconf, "Contact", "dealerphonenumber");
    addFieldToDbConf(dataObj, &dbconf, "Mobile_No", "dealermobilenumber");
    addFieldToDbConf(dataObj, &dbconf, "Email_addr", "dealeremail");

    fccboscom::RoData rodata;
    rodata.set_msgid(1);
    *rodata.add_dbconf() = dbconf;

    bool success = serializeAndSendToFcc(rodata, "Set_RoConfiguration", apiVersion);
    return cbosparser::generateResponse("Set_RoConfiguration", apiVersion, success);
}



string handleSetProductConfiguration(const json& parsed, int apiVersion) {
    const auto& dataObj = parsed.value("Data", json::object());
    const auto& productList = dataObj.value("product_List", json::array());

    bool allSuccess = true;

    // Clear the map at start
    cbosparser::productNameToIdMap.clear();

    for (const auto& product : productList) {
        fccboscom::RoData rodata;
        rodata.set_msgid(1);

        fccboscom::DbConf dbconf;
        dbconf.set_tablename("public.product");
        dbconf.set_operation(1);

        int pid = product.value("P_Id", 0);
        std::string pidStr = std::to_string(pid);

        std::string productName = product.value("P_Name", "");
        std::string productCode = product.value("P_Code", "");
       

        addHardcodedFieldToDbConf(&dbconf, "id", pidStr, 1);
        addHardcodedFieldToDbConf(&dbconf, "uid", pidStr, 1);

        addHardcodedFieldToDbConf(&dbconf, "prodname","'" + productName + "'", 1);
        addHardcodedFieldToDbConf(&dbconf, "prodcode", "'" + productCode + "'", 1);
        addHardcodedFieldToDbConf(&dbconf, "isenable", "1", 2);

        // Save mapping for lookup later
        if (!productName.empty()) {
            cbosparser::productNameToIdMap[productName] = pid;  //mapped to the unorderedmap
            std::cerr << "[Mapping] Added mapping: '" << productName << "' -> " << pid << "\n";
        } else {
            std::cerr << "[Warning] Empty product name for product ID " << pid << "\n";
        }

        *rodata.add_dbconf() = dbconf;

        bool success = serializeAndSendToFcc(rodata, "Set_ProductConfiguration", apiVersion);
        if (!success) {
            std::cerr << "[Error] Failed to send product configuration for product ID " << pid << "\n";
            allSuccess = false;
        }
    }

    return cbosparser::generateResponse("Set_ProductConfiguration", apiVersion, allSuccess);
}



string handleSetGradeConfiguration(const json& parsed, int apiVersion) {
    const auto& dataObj = parsed.value("Data", json::object());
    const auto& GradeList = dataObj.value("Grade_List", json::array());

    bool allSuccess = true;

    for (const auto& grade : GradeList) {
        fccboscom::RoData rodata;
        rodata.set_msgid(1);

        fccboscom::DbConf dbconf;
        dbconf.set_tablename("public.grade");
        dbconf.set_operation(1);

        int gid = grade.value("G_Id", 0);
        std::string gidStr = std::to_string(gid);

        addHardcodedFieldToDbConf(&dbconf, "id", gidStr, 1);
        addHardcodedFieldToDbConf(&dbconf, "uid", gidStr, 1);

        addFieldToDbConf(grade, &dbconf, "G_Name", "gradename");
        addFieldToDbConf(grade, &dbconf, "G_Price", "price");
        addFieldToDbConf(grade, &dbconf, "PHP", "highproductpercentage");
        addFieldToDbConf(grade, &dbconf, "PLP", "lowproductpercentage");

        std::string gName = grade.value("G_Name", "");
        auto it = cbosparser::productNameToIdMap.find(gName);

        if (it != cbosparser::productNameToIdMap.end()) {
            std::string productUid = std::to_string(it->second);
            addHardcodedFieldToDbConf(&dbconf, "productuid", productUid, 1);
            std::cerr << "[INFO] Mapping grade '" << gName << "' to productuid = " << productUid << std::endl;
        } else {
            std::cerr << "[ERROR] Product name '" << gName << "' not found in productNameToIdMap!" << std::endl;
            allSuccess = false;
            continue;
        }

        addHardcodedFieldToDbConf(&dbconf, "isenable", "1", 2);

        *rodata.add_dbconf() = dbconf;

        bool success = serializeAndSendToFcc(rodata, "Set_GradeConfiguration", apiVersion);
        if (!success) {
            std::cerr << "[ERROR] serializeAndSendToFcc failed for Grade UID=" << gidStr << std::endl;
            allSuccess = false;
        } else {
            std::cerr << "[INFO] Successfully sent grade config for UID=" << gidStr << std::endl;
        }
    }

    std::string response = cbosparser::generateResponse("Set_GradeConfiguration", apiVersion, allSuccess);
    std::cerr << "[INFO] Response: " << response << std::endl;

    return response;
}


string handleSetTankConfiguration(const json& parsed, int apiVersion) {
    const auto& dataObj = parsed.value("Data", json::object());
    const auto& TankList = dataObj.value("Tank_List", json::array());

    bool allSuccess = true;

    cbosparser::tankNoToIdMap.clear();

    for (const auto& tank : TankList) {
        fccboscom::RoData rodata;
        rodata.set_msgid(1);

        fccboscom::DbConf dbconf;
        dbconf.set_tablename("public.tank");
        dbconf.set_operation(1);
        
      
        int tno = tank.value("T_No", 0);
        std::string tnoStr = std::to_string(tno);

        std::cerr << "[Tank]" << tno << "\n";

        addHardcodedFieldToDbConf(&dbconf, "id", tnoStr, 1);
        addHardcodedFieldToDbConf(&dbconf, "uid", tnoStr, 1);


        addFieldToDbConf(tank, &dbconf, "T_PId", "productuid");
        addFieldToDbConf(tank, &dbconf, "T_Height", "tankdip");
        addFieldToDbConf(tank, &dbconf, "T_Capacity", "tankvolume");
        addFieldToDbConf(tank, &dbconf, "HW", "tankmaxwater");
        addFieldToDbConf(tank, &dbconf, "TLP", "tankminvolume");
        addFieldToDbConf(tank, &dbconf, "Over_Capacity", "tankmaxvolume");

        addHardcodedFieldToDbConf(&dbconf, "probetypeuid", tnoStr, 1);
        addHardcodedFieldToDbConf(&dbconf, "atguid", "1", 2);
        addHardcodedFieldToDbConf(&dbconf, "prodlevel", "0", 3);
        addHardcodedFieldToDbConf(&dbconf, "prodvolume", "0", 3);
        addHardcodedFieldToDbConf(&dbconf, "waterlevel", "0", 3);
        addHardcodedFieldToDbConf(&dbconf, "watervolume", "0", 3);
        addHardcodedFieldToDbConf(&dbconf, "prodtcvolume", "0", 3);
        addHardcodedFieldToDbConf(&dbconf, "ullage", "0", 3);
        addHardcodedFieldToDbConf(&dbconf, "temperature", "0", 3);
        addHardcodedFieldToDbConf(&dbconf, "density", "0", 3);
        addHardcodedFieldToDbConf(&dbconf, "tcdensity", "0", 3);
        addHardcodedFieldToDbConf(&dbconf, "commtimeout", "180", 3);
        addHardcodedFieldToDbConf(&dbconf, "commfaillock", "0", 2);
        addHardcodedFieldToDbConf(&dbconf, "prodlowlock", "0", 2);
        addHardcodedFieldToDbConf(&dbconf, "waterhighlock", "0", 2);
        addHardcodedFieldToDbConf(&dbconf, "ttreceiptlock", "0", 2);
        addHardcodedFieldToDbConf(&dbconf, "deliverylock", "0", 2);
        addHardcodedFieldToDbConf(&dbconf, "isenable", "1", 2);

        cbosparser::tankNoToIdMap[tno] = tno;
        std::cerr <<"[Mapping] Added T_No " << tno << " id" << tno << "\n";

        *rodata.add_dbconf() = dbconf;

        // Send the message immediately for this tank
        bool success = serializeAndSendToFcc(rodata, "Set_TankConfiguration", apiVersion);
        if (!success) {
            allSuccess = false;
            // Optionally, log or handle failure for this tank here
        }
    }

    return cbosparser::generateResponse("Set_TankConfiguration", apiVersion, allSuccess);
}



std::string handleSetDispenserConfiguration(const json& parsed, int apiVersion) {
    const auto& dataObj = parsed["Data"];

    int dispenserNo = dataObj.value("D_No", 0);
    std::string serialNo = dataObj.value("D_SerialNo", "");
    std::string paraHex = dataObj.value("D_Para", "");
    std::string paraDecimal = convertHexParaToDecimalString(paraHex);
    const auto& pumpList = dataObj["Pump_List"];

    bool success = true;
    int firstFpType = !pumpList.empty() ? pumpList[0].value("Fp_Type", 0) : 0;

    // Lookup dispenser info
    DispenserInfo dispenserInfo = getDispenserInfo(firstFpType);

    // Devicemake string to int mapping
    static const std::unordered_map<std::string, int> devicemakeMap = {
        {"Tokheim", 1},
        {"Gilbarco", 2},
        {"Midco", 3},
        {"Wayne", 4},
        {"Tatsuno", 8}
    };

    int devicemakeId = devicemakeMap.count(dispenserInfo.make) ? devicemakeMap.at(dispenserInfo.make) : 0;

    // --- SlaveConfig Message ---
    fccboscom::RoData rodataSlave;
    rodataSlave.set_msgid(1);

    fccboscom::DbConf slaveConf;
    slaveConf.set_tablename("public.slaveconfig");
    slaveConf.set_operation(1);

    auto config = readConfigFile("Config/Config.properties");
    std::string comportid = config.count("slavecomportid") ? config["slavecomportid"] : "0";

    addHardcodedFieldToDbConf(&slaveConf, "id", std::to_string(dispenserNo), 1);
    addHardcodedFieldToDbConf(&slaveConf, "address", std::to_string(dispenserNo), 1);
    addHardcodedFieldToDbConf(&slaveConf, "devicetype", "0", 1);
    addHardcodedFieldToDbConf(&slaveConf, "comportid",comportid, 1);
    addHardcodedFieldToDbConf(&slaveConf, "devicemake", std::to_string(devicemakeId),1);
    addHardcodedFieldToDbConf(&slaveConf, "isenable", "1", 1);
    addHardcodedFieldToDbConf(&slaveConf, "updatetime", "'" + getCurrentTimestamp() + "'", 4);
    addHardcodedFieldToDbConf(&slaveConf, "firmwareversion", std::to_string(dispenserInfo.firmwareVersion), 1);
    addHardcodedFieldToDbConf(&slaveConf, "macaddress", paraDecimal, 2);
    addHardcodedFieldToDbConf(&slaveConf, "timeout", "100", 1);

    *rodataSlave.add_dbconf() = slaveConf;

    bool slaveSent = serializeAndSendToFcc(rodataSlave, "Set_DispenserConfiguration_Slave", apiVersion);
    if (!slaveSent) {
        std::cerr << "[ERROR] Failed to send slaveconfig AMQP message" << std::endl;
    }
    success &= slaveSent; 

    // --- Pump & Nozzle Processing ---
    for (const auto& pump : pumpList) {
        int fpNo = pump.value("FP_No", 0);
        int fpType = pump.value("Fp_Type", 0);
        int phyAddr = pump.value("Phy_Addr", 0);

        DispenserInfo info = getDispenserInfo(fpType);

        int nozzleCount = pump["Nozzle_List"].size();
        int  VDP = std::stod(pump.value("VDP", "0"));
        int  ADP = std::stod(pump.value("ADP", "0"));
        int  MDP = std::stod(pump.value("MDP", "0"));
        int  PDP = std::stod(pump.value("PDP", "0"));
        double Max_TV = std::stod(pump.value("Max_TV", "0"));
        double Max_TA = std::stod(pump.value("Max_TA", "0"));

        fccboscom::RoData rodataPump;
        rodataPump.set_msgid(1);

        fccboscom::DbConf pumpConf;
        pumpConf.set_tablename("public.dispenserpump");
        pumpConf.set_operation(1);

        addHardcodedFieldToDbConf(&pumpConf, "id", std::to_string(fpNo), 1);
        addHardcodedFieldToDbConf(&pumpConf, "uid", std::to_string(fpNo), 1);
        addHardcodedFieldToDbConf(&pumpConf, "duid", std::to_string(dispenserNo), 1);
        addHardcodedFieldToDbConf(&pumpConf, "duuid", std::to_string(dispenserNo), 1);
        addHardcodedFieldToDbConf(&pumpConf, "make", info.make, 2);
        addHardcodedFieldToDbConf(&pumpConf, "model", info.model, 2);
        addHardcodedFieldToDbConf(&pumpConf, "pversion", info.pversion, 2);
        addHardcodedFieldToDbConf(&pumpConf, "phyaddress", std::to_string(phyAddr), 1);
        addHardcodedFieldToDbConf(&pumpConf, "noofnozzles", std::to_string(nozzleCount), 1);
        addHardcodedFieldToDbConf(&pumpConf, "vdp", std::to_string(VDP), 3);
        addHardcodedFieldToDbConf(&pumpConf, "adp", std::to_string(ADP), 3);
        addHardcodedFieldToDbConf(&pumpConf, "mdp", std::to_string(MDP), 3);
        addHardcodedFieldToDbConf(&pumpConf, "pdp", std::to_string(PDP), 3);
        addHardcodedFieldToDbConf(&pumpConf, "maxvolume", std::to_string(Max_TV), 3);
        addHardcodedFieldToDbConf(&pumpConf, "maxamount", std::to_string(Max_TA), 3);
        addHardcodedFieldToDbConf(&pumpConf, "pumpmode", "1", 1);
        addHardcodedFieldToDbConf(&pumpConf, "isenable", "1", 1);
        addHardcodedFieldToDbConf(&pumpConf, "slaveid", std::to_string(dispenserNo), 1);

        *rodataPump.add_dbconf() = pumpConf;
        success &= serializeAndSendToFcc(rodataPump, "Set_DispenserConfiguration_Pump", apiVersion);

        int fpIndex = fpNo - 1; 

int nozzleIndex = 1; // Start nozzle index from 1

for (const auto& nozzle : pump["Nozzle_List"]) {
    int nozId = nozzle.value("Noz_Id", 0);
    int pId   = nozzle.value("P_Id", 0);
    int tNo   = nozzle.value("T_No", 0);

    // Calculate the desired id:
    int id = fpIndex * 1 + nozzleIndex;

    fccboscom::RoData rodataNozzle;
    rodataNozzle.set_msgid(1);

    fccboscom::DbConf nozzleConf;
    nozzleConf.set_tablename("public.nozzleproductmap");
    nozzleConf.set_operation(1);

    // Use the calculated id
    addHardcodedFieldToDbConf(&nozzleConf, "id", std::to_string(id), 1);
    addHardcodedFieldToDbConf(&nozzleConf, "pumpuid", std::to_string(fpNo), 1);
    addHardcodedFieldToDbConf(&nozzleConf, "nozzleid", std::to_string(nozId), 1);
    addHardcodedFieldToDbConf(&nozzleConf, "tankuid", std::to_string(tNo), 1);
    addHardcodedFieldToDbConf(&nozzleConf, "gradeuid", std::to_string(pId), 1);
    addHardcodedFieldToDbConf(&nozzleConf, "isenable", "1", 1);

    *rodataNozzle.add_dbconf() = nozzleConf;
    success &= serializeAndSendToFcc(rodataNozzle, "Set_DispenserConfiguration_Nozzle", apiVersion);

    nozzleIndex++; // Increment for next nozzle
}

    }

    return cbosparser::generateResponse("Set_DispenserConfiguration", apiVersion, success);
}




string cbosparser::generateResponse(const string& apiName, int version, bool isValid) {
    json response = {
        {"Api_Name", apiName},
        {"Api_Version", version},
        {"Is_Valid", isValid}
    };

    char STX = 0x02;
    char ETX = 0x03;
    return string(1, STX) + response.dump() + string(1, ETX);
}

