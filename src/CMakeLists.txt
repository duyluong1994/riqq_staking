project(riqq_staking)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( riqq_staking riqq_staking riqq_staking.cpp )
target_include_directories( riqq_staking PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( riqq_staking ${CMAKE_SOURCE_DIR}/../ricardian )