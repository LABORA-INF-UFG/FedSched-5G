build_lib_example(
    NAME ns3ai_fl_gym
    SOURCE_FILES
        sim.cc
        fl-gym-env.cc
        fl-client.cc
        fl-server.cc
    LIBRARIES_TO_LINK
        ${libai}
        ${libcore}
        ${libnetwork}
        ${libinternet}
        ${libapplications}
        ${libmobility}
        ${libantenna}
        ${libbuildings}
        ${libnr}
        ${libpoint-to-point}
        ${libflow-monitor}
)

