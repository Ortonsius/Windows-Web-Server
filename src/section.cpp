std::string CheckDepencies(json data){
    if(data["general"]["engine-type"] == EnType::HTTP_SERVER){
        // Header
        if(!data.contains("header")) return "Missing Header";
        if(!data["header"].contains("ip")) return "Missing IP";
        if(!data["header"].contains("port")) return "Missing Port";
        if(!data["header"].contains("protocol")) return "Missing Protocol";
        if(!data["header"].contains("rto")) return "Missing Read-Timeout";
        if(!data["header"].contains("wto")) return "Missing Write-Timeout";
        if(!data["header"].contains("mbs")) return "Missing Max-Bufffer-Size";
        if(!data["header"].contains("logr")) return "Missing Log-Request";
        if(!data["header"].contains("loge")) return "Missing Log-Error";
        if(!data["header"].contains("cs")) return "Missing Chunk Size";
        if(!data["header"].contains("backlog")) return "Missing Backlog";

        // Events
        if(!data.contains("events")) return "Missing Events";
        if(!data["events"].contains("pre-process")) return "Missing PreProcess";
        if(!data["events"].contains("post-process")) return "Missing PostProcess";

        // URL
        if(!data.contains("url")) return "Missing URL";

        // Plugin
        if(!data.contains("plugins")) return "Missing Plugin";
    }else{
        return "Undefined engine";
    }
    return "";
}

std::string SectionHTTPServer(json data){
    // Check all depencies
    std::string cd = CheckDepencies(data);
    if(cd != ""){
        return cd;
    }

    HTTPServer engine;
    engine.Name = data["general"]["name"].get<std::string>();
    engine.IP = data["header"]["ip"].get<std::string>();
    engine.Port = data["header"]["port"].get<int>();
    engine.Protocol = data["header"]["protocol"].get<int>();
    engine.ReadTimeout = data["header"]["rto"].get<int>();
    engine.WriteTimeout = data["header"]["rto"].get<int>();
    engine.MaxBufferSize = data["header"]["mbs"].get<double>() * (double)1048576;
    engine.ChunkSize = data["header"]["cs"].get<int>();
    engine.Backlog = data["header"]["backlog"].get<int>();
    engine.Loge = data["header"]["loge"].get<bool>();
    engine.Logr = data["header"]["logr"].get<bool>();
    engine.Plugins = data["plugins"].get<std::vector<std::string>>();

    for(auto i = data["url"].begin(); i != data["url"].end();++i){
        if(data["url"][i.key()].contains("behaviour")){
            if(data["url"][i.key()]["behaviour"] == "s"){
                engine.StaticURL[i.key()] = i.value();
            }else if(data["url"][i.key()]["behaviour"] == "d"){
                engine.DynamicURL[i.key()] = i.value();
            }else{
                return "Invalid URL structure";
            }
        }else{
            return "Invalid URL structure";
        }
    }
    engine.Events = data["events"];
    engine.WebContentType = SetupWCT();


    std::string err = engine.Setup();
    if(err != "") return err;
    engine.Run();
    return "Server shutdown unexpectedly";
}