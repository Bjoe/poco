add_executable(SetSourceIP src/SetSourceIP.cpp)
target_link_libraries(SetSourceIP Poco::NetSSL Poco::Crypto Poco::Util Poco::Net Poco::XML Poco::Foundation)
