//#include <darknet.h>
#include <fdeep/fdeep.hpp>
#include <lorina/lorina.hpp>
#include <alice/alice.hpp>
#include <kitty/kitty.hpp>
#include <mockturtle/mockturtle.hpp>
#include <oracle/oracle.hpp>
#include <libkahypar.h>

#include <mockturtle/views/mffc_view.hpp>
#include <mockturtle/views/fanout_view.hpp>

#include <iostream>
#include <string>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <vector>
#include <time.h>
#include <fmt/format.h>
#include <stdio.h>
#include <stdlib.h>
#include <aig/gia/gia.h>
#include <aig/gia/giaAig.h>
#include <base/wlc/wlc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iomanip>
#include <ot/timer/timer.hpp>

namespace alice{

  //object to hold STA configuration
  STA sta_cfg;

  int computeLevel( mockturtle::aig_network aig, int index ) {
	 //if node not visited
    if(aig._storage->nodes[index].data[1].h1==0) {

      //set node as visited
      aig._storage->nodes[index].data[1].h1=1;

      //if is input
      if (aig.is_ci(index)) {
          return 0;
      }

  		auto inIdx2 = aig._storage->nodes[index].children[1].data;
  		if (inIdx2 & 1)
  			inIdx2 = inIdx2 - 1;

  		//calculate input node index
  		auto inNode1 = inIdx2 >> 1;
  		int levelNode1 = computeLevel(aig, inNode1);

      auto inIdx = aig._storage->nodes[index].children[0].data;
      if (inIdx & 1)
          inIdx = inIdx - 1;

      //calculate input node index
      auto inNode0 = inIdx >> 1;
      int levelNode0 = computeLevel(aig, inNode0);

      int level = 1 + std::max(levelNode0, levelNode1);
      return level;
    } 
  }

  void dfs (mockturtle::aig_network aig, uint64_t index, UnionFind uf){
    if(aig._storage->nodes[index].data[1].h1==0){
		  //set node as visited
      aig._storage->nodes[index].data[1].h1=1;

      //traverse the left nodes to the PIs
      if(!aig.is_pi(aig._storage->nodes[index].children[0].index) && index > aig.num_pis() ){
      	auto inData0 = aig._storage->nodes[index].children[0].data;
      	//treat inverted and inputs
  			if( inData0 & 1)
  				inData0 = aig._storage->nodes[index].children[0].data - 1;

        //calculate input node index
        auto inIndex = inData0 >> 1;

        //ignores latches and PIs
        if(inIndex > aig.num_pis()) {
  				uf.merge(index, inIndex);
  				dfs(aig, inIndex, uf);
  			}
      }

  		//traverse the right nodes to the PIs
  		if(!aig.is_pi(aig._storage->nodes[index].children[1].index) && index > aig.num_pis() ){
  			auto inData1 = aig._storage->nodes[index].children[1].data;
  			if( inData1 & 1)
  				inData1 = aig._storage->nodes[index].children[1].data - 1;

  			//calculate input node index
  			auto inIndex = inData1 >> 1;

  			//im ignoring latches
  			if(inIndex > aig.num_pis()) {
  				uf.merge(index, inIndex);
  				dfs(aig, inIndex, uf);
  			}
  		}
    }
  }

  void compute_cone(mockturtle::aig_network aig, uint64_t index, std::unordered_map<int, int> &nodes, int outindex, std::unordered_map<int, int> &ins){
  	if(aig._storage->nodes[index].data[1].h1==0){

  		//increment number of nodes in this cone
  		std::unordered_map<int, int>::iterator it = nodes.find(outindex);

  		if(it!=nodes.end() && index > aig.num_pis()) {
  			//increment the number of nodes
  			it->second++;
  		}

  		//set node as visited
  		aig._storage->nodes[index].data[1].h1=1;

  		//traverse one side to the PIs
  		if(!aig.is_pi(aig._storage->nodes[index].children[0].index) && index > aig.num_pis() ){
  			if(aig._storage->nodes[index].children[0].data & 1)
  				aig._storage->nodes[index].children[0].data = aig._storage->nodes[index].children[0].data - 1;

  			//calculate input node index
  			auto inIndex = aig._storage->nodes[index].children[0].data >> 1;

  			//im ignoring latches
  			if(inIndex > aig.num_pis()) {
  				//call recursion
  				compute_cone(aig, inIndex, nodes, outindex, ins);
  			}
  		}

  		//traverse the other side to the PIs
  		if(!aig.is_pi(aig._storage->nodes[index].children[1].index)&& index > aig.num_pis() ){
  			if(aig._storage->nodes[index].children[1].data & 1)
  				aig._storage->nodes[index].children[1].data = aig._storage->nodes[index].children[1].data - 1;

  			//calculate input node index
  			auto inIndex = aig._storage->nodes[index].children[1].data >> 1;

  			//im ignoring latches
  			if(inIndex > aig.num_pis()) {
  				//call recursion
  				compute_cone(aig, inIndex, nodes, outindex, ins);
  			}
  		}

  		//if my child is PI and was not visited yet, I increase the input counter
  		if(aig.is_ci(aig._storage->nodes[index].children[0].index) && aig._storage->nodes[aig._storage->nodes[index].children[0].index].data[1].h1 == 0 ){
  			aig._storage->nodes[aig._storage->nodes[index].children[0].index].data[1].h1 = 1;

  			std::unordered_map<int, int>::iterator it = ins.find(outindex);
  			if(it!=ins.end()) {
  				//increment the number of inputs
  				it->second++;
  			}
  		}

  		//if my other child is PI and was not visited yet, I also increase the input counter
  		if(aig.is_ci(aig._storage->nodes[index].children[1].index) && aig._storage->nodes[aig._storage->nodes[index].children[1].index].data[1].h1 == 0){
  			aig._storage->nodes[aig._storage->nodes[index].children[1].index].data[1].h1 = 1;

  			std::unordered_map<int, int>::iterator it = ins.find(outindex);
  			if(it!=ins.end()) {
  				//increment the number of inputs
  				it->second++;
  			}
  		}
  	}
  }

  /* Adds And-inverter graphs (Mockturtle type aig_network) as store element type to
   * alice.
   *
   * One can access AIGs in general store commands using the long --aig flag or
   * the short -a flag.
   */
  ALICE_ADD_STORE( mockturtle::aig_network, "aig", "a", "aig", "AIGs" )

  /* Implements the short string to describe a store element in store -a */
  ALICE_DESCRIBE_STORE( mockturtle::aig_network, aig ){

    const auto name = "aig_placeholder";
    const auto pi_num = aig.num_pis();
    const auto po_num = aig.num_pos();
    return fmt::format( "{} i/o = {}/{}", name, pi_num, po_num );
  }//end aig_network describe store

  ALICE_LOG_STORE_STATISTICS( mockturtle::aig_network, aig){

    return {
    	{"nodes", aig.size()},
    	{"inputs", aig.num_pis() - aig.num_latches()},
    	{"latches", aig.num_latches()},
    	{"outputs", aig.num_pos() - aig.num_latches()},
    	{"AIG nodes", aig.num_gates()}};
  }//end aig_network log store statistics

  /* Implements the functionality of ps -b */
  ALICE_PRINT_STORE_STATISTICS( mockturtle::aig_network, os, aig ){
  	os << "nodes: " << aig.size() << std::endl;
  	os << "inputs: " << aig.num_pis() - aig.num_latches() << std::endl;
  	os << "latches: " << aig.num_latches() << std::endl;
  	os << "outputs: " << aig.num_pos() - aig.num_latches() << std::endl;
  	os << "AIG nodes: " << aig.num_gates() << std::endl;

  }//end aig_network print store statistics

  ALICE_ADD_STORE( mockturtle::mig_network, "mig", "m", "mig", "MIGs" )

  /* Implements the short string to describe a store element in store -a */
  ALICE_DESCRIBE_STORE( mockturtle::mig_network, mig ){

  	const auto name = "mig_placeholder";
  	const auto pi_num = mig.num_pis();
  	const auto po_num = mig.num_pos();

  	return fmt::format( "{} i/o = {}/{}", name, pi_num, po_num );
  }//end aig_network describe store

  ALICE_LOG_STORE_STATISTICS( mockturtle::mig_network, mig){

  	return {
  			{"nodes", mig.size()},
  			{"inputs", mig.num_pis() - mig.num_latches()},
  			{"latches", mig.num_latches()},
  			{"outputs", mig.num_pos() - mig.num_latches()},
  			{"MAJ nodes", mig.num_gates()}};
  }//end aig_network log store statistics

  /* Implements the functionality of ps -b */
  ALICE_PRINT_STORE_STATISTICS( mockturtle::mig_network, os, mig ){
  	os << "nodes: " << mig.size() << std::endl;
  	os << "inputs: " << mig.num_pis() - mig.num_latches() << std::endl;
  	os << "latches: " << mig.num_latches() << std::endl;
  	os << "outputs: " << mig.num_pos() - mig.num_latches()<< std::endl;
  	os << "MAJ nodes: " << mig.num_gates() << std::endl;
  }//end aig_network print store statistics

  ALICE_ADD_STORE( oracle::partition_manager<mockturtle::mig_network>, "part_man_mig", "pm_m", "part_man_mig", "PART_MAN_MIGs")

  /* Implements the short string to describe a store element in store -a */
  ALICE_DESCRIBE_STORE( oracle::partition_manager<mockturtle::mig_network>, part_man ){

    const auto name = "partition manager for MIG networks";
    const auto part_num = part_man.get_part_num();

    return fmt::format( "{} # partitions = {}", name, part_num );
  }//end partition manager<mig_network> describe store

  ALICE_LOG_STORE_STATISTICS( oracle::partition_manager<mockturtle::mig_network>, part_man){

    return {
            {"partition number", part_man.get_part_num()}};
  }//end partition manager<mig_network> log store statistics

  /* Implements the functionality of ps -b */
  ALICE_PRINT_STORE_STATISTICS( oracle::partition_manager<mockturtle::mig_network>, os, part_man ){
    os << "partition number: " << part_man.get_part_num() << std::endl;
  }//end partition manager<mig_network> print store statistics

  ALICE_ADD_STORE( oracle::partition_manager<mockturtle::aig_network>, "part_man_aig", "pm_a", "part_man_aig", "PART_MAN_AIGs")

  /* Implements the short string to describe a store element in store -a */
  ALICE_DESCRIBE_STORE( oracle::partition_manager<mockturtle::aig_network>, part_man ){

    const auto name = "partition manager for AIG networks";
    const auto part_num = part_man.get_part_num();

    return fmt::format( "{} # partitions = {}", name, part_num );
  }//end partition manager<aig_network> describe store

  ALICE_LOG_STORE_STATISTICS( oracle::partition_manager<mockturtle::aig_network>, part_man){

    return {
            {"partition number", part_man.get_part_num()}};
  }//end partition manager<aig_network> log store statistics

  /* Implements the functionality of ps -b */
  ALICE_PRINT_STORE_STATISTICS( oracle::partition_manager<mockturtle::aig_network>, os, part_man ){
    os << "partition number: " << part_man.get_part_num() << std::endl;
  }//end partition manager<aig_network> print store statistics

  ALICE_ADD_STORE(mockturtle::klut_network, "bench", "b", "BENCH", "BENCHes")

  ALICE_DESCRIBE_STORE(mockturtle::klut_network, bench){
    const auto name = "bench_placeholder";
    const auto pi_num = bench.num_pis();
    const auto po_num = bench.num_pos();
    return fmt::format( "{} i/o = {}/{}", name, pi_num, po_num );
  }//end klut_network describe store

  ALICE_LOG_STORE_STATISTICS( mockturtle::klut_network, bench){

    return {
        {"inputs", bench.num_pis()},
        {"outputs", bench.num_pos()},
        {"nodes", bench.size()},
        {"gates", bench.num_gates()}};
  }//end klut_network log store statistics

  /* Implements the functionality of ps -b */
  ALICE_PRINT_STORE_STATISTICS( mockturtle::klut_network, os, bench ){

    os << "inputs: " << bench.num_pis() << std::endl;
    os << "outputs: " << bench.num_pos() << std::endl;
    os << "nodes: " << bench.size() << std::endl;
    os << "gates: " << bench.num_gates() << std::endl;
  }//end klut_network print store statistics

  ALICE_ADD_STORE(abc::Gia_Man_t*, "gia", "g", "AIG", "AIGs")

  /* Implements the short string to describe a store element in store -a */
  ALICE_DESCRIBE_STORE( abc::Gia_Man_t*, gia ){

    const auto name = abc::Gia_ManName( gia );
    const auto pi_num = abc::Gia_ManPiNum( gia );
    const auto po_num = abc::Gia_ManPoNum( gia );
    return fmt::format( "{} i/o = {}/{}", name, pi_num, po_num );
  }

  /* Implements the functionality of ps -a */
  ALICE_PRINT_STORE_STATISTICS( abc::Gia_Man_t*, os, gia ){

    abc::Gps_Par_t Pars{};
    abc::Gia_ManPrintStats( gia, &Pars );
  }

  /* Implements the log returned by ps -a */
  ALICE_LOG_STORE_STATISTICS( abc::Gia_Man_t*, gia ){

    return {
            {"name", abc::Gia_ManName( gia )},
            {"inputs", abc::Gia_ManPiNum( gia )},
            {"outputs", abc::Gia_ManPoNum( gia )},
            {"nodes", abc::Gia_ManAndNum( gia )},
            {"latches"}, abc::Gia_ManRegNum(gia),
            {"levels", abc::Gia_ManLevelNum( gia )}};
  }

  ALICE_ADD_FILE_TYPE( aiger, "Aiger" )

  /* Implements the functionality of read_aiger -a */
  ALICE_READ_FILE( abc::Gia_Man_t*, aiger, filename, cmd ){

    return abc::Gia_AigerRead( (char*)filename.c_str(), 0, 0, 0 );
  }

  /* Implements the functionality of write_aiger -a */
  ALICE_WRITE_FILE( abc::Gia_Man_t*, aiger, gia, filename, cmd ){

    abc::Gia_AigerWrite( gia, (char*)filename.c_str(), 1, 0 );
  }

  /* Implements the command syn3 */
  ALICE_COMMAND( syn3, "Optimization", "Performs AIG optimization" ){

    auto& gia = store<abc::Gia_Man_t*>().current();

    auto aig_new = abc::Gia_ManAigSyn3( gia, 0, 0 );
    abc::Gia_ManStop( gia );
    gia = aig_new;
  }

  /*Reads an blif file and stores the CBA network in a store*/
  ALICE_COMMAND( get_blif, "Input", "Uses the lorina library to read in a blif file" ){

  	std::cout << "Not yet fully implemented\n";	
  }//end get_blif

  ALICE_COMMAND( convert_aig, "Input", "Converts the stored AIG to MIG and writes out to verilog" ){

    if(!store<mockturtle::aig_network>().empty()){
        auto aig = store<mockturtle::aig_network>().current();
        mockturtle::mig_network mig = aig_to_mig(aig);
        mockturtle::write_verilog(mig, aig._storage->net_name + "_convert.v");
    }  
  }//end get_blif

  class write_aig_command : public alice::command{

    public:
      explicit write_aig_command( const environment::ptr& env )
          : command( env, "Writes the AIG in the ASCII format" ){

        opts.add_option( "--filename,filename", filename, "AAG file to write stored AIG to" )->required();
      }

    protected:
      void execute(){
        if(checkExt(filename, "aag")){
          if(!store<mockturtle::aig_network>().empty()){
            auto aig = store<mockturtle::aig_network>().current();

            std::ofstream aigfile;
            aigfile.open (filename);

            // header info - MILOA
            auto _num_inputs = aig.num_pis();
            auto _num_latches = aig.num_latches();
            auto _num_outputs = aig.num_pos();
            auto _num_ands = aig.num_gates();
            auto _num_vertices = aig.num_pis() + aig.num_gates();

            //write aig header
            aigfile << "aag " << _num_vertices << " " << _num_inputs - _num_latches << " " << _num_latches << " " << (_num_outputs - _num_latches) << " " << _num_ands << std::endl;

            aig.foreach_pi([&] (auto node){
              auto index = aig.pi_index(node);
              aigfile << (index+1)*2 << "\n";
            });

            //write aig latches
            auto lineIdx = ((aig.num_pis()-aig.num_latches())*2)+2;
            for(int i=0; i<aig.num_latches(); i++){
              auto regIdx = aig.num_pos() - aig.num_latches() + i;
              aigfile << lineIdx << " " << aig._storage->outputs[regIdx].data << "\n";
              lineIdx += 2;
            }

            //write aig outputs
            for(int i=0; i<aig.num_pos()- aig.num_latches(); i++){
              aigfile << aig._storage->outputs[i].data << "\n";
            }

            auto skipLatches = 0;
            aig.foreach_gate([&] (auto node){
              //skip latches in the nodes vector
              if(skipLatches>=aig.num_latches()) {
                auto index = aig.node_to_index(node);
                auto left = aig._storage->nodes[index].children[0].data;
                auto right = aig._storage->nodes[index].children[1].data;
                aigfile << index*2 << " " << left << " " << right << "\n";
              }
              skipLatches+=1;
            });

            for(int i = 0; i < aig._storage->inputs.size(); i++){
              aigfile << "i" << i << " " << aig._storage->inputNames[aig._storage->inputs.at(i) - 1] << "\n";
            }
            for(int i = 0; i < aig._storage->outputs.size(); i++){
              aigfile << "o" << i << " " << aig._storage->outputNames[i] << "\n";
            }

            aigfile.close();
          }
          else{
              std::cout << "No AIG stored\n";
          }
        }
        else{
          std::cout << "File not a vlid aag file\n";
        }
      }
    private:
      std::string filename{};
    };

  ALICE_ADD_COMMAND(write_aig, "Output");

  class write_to_bench_command : public alice::command{

    public:
      explicit write_to_bench_command( const environment::ptr& env )
          : command( env, "Writes a stored file to an external bench file" ){

        opts.add_option( "--filename,filename", filename, "bench file to write to" )->required();
        add_flag("--mig,-m", "Write the stored MIG file to a bench file");
        add_flag("--bench,-b", "Write the stored bench file to a bench file");
      }

    protected:
      void execute(){

        if(is_set("mig")){
        	mockturtle::mig_network mig = store<mockturtle::mig_network>().current();
    	    mockturtle::write_bench(mig, filename);
      	}
      	else if(is_set("bench")){
      		mockturtle::klut_network bench = store<mockturtle::klut_network>().current();
      		mockturtle::write_bench(bench, filename);
      	}
      	else{
      		std::cout << "Store to write out wasn't specified\n";
      	}
      }
    private:
      std::string filename{};
    };

  ALICE_ADD_COMMAND(write_to_bench, "Output");

  class read_verilog_command : public alice::command{

    public:
      explicit read_verilog_command( const environment::ptr& env )
          : command( env, "Uses the lorina library to read in an Verilog file" ){

        opts.add_option( "--filename,filename", filename, "Verilog file to read in" )->required();
        add_flag("--mig,-m", "Store Verilog file as MIG network (AIG network is default)");
      }

    protected:
      void execute(){

        if(checkExt(filename, "v")){
          if(is_set("mig")){
            mockturtle::mig_network mig;
            lorina::read_verilog(filename, mockturtle::verilog_reader( mig ));
            std::cout << "MIG network stored" << std::endl;
            store<mockturtle::mig_network>().extend() = mig;


            if(mig._storage->inputNames.size() == 0){
              for(int i = 0; i < mig.num_pis(); i++){
                std::string input_name = "pi";
                input_name.append(std::to_string(i));
                mig._storage->inputNames[i] = input_name;
              }
            }
            if(mig._storage->outputNames.size() == 0){
              for(int i = 0; i < mig.num_pos(); i++){
                std::string output_name = "po";
                output_name.append(std::to_string(i));
                mig._storage->outputNames[i] = output_name;
              }
            }

            filename.erase(filename.end() - 4, filename.end());
            mig._storage->net_name = filename;
          }
          else{
            mockturtle::aig_network aig;
            lorina::read_verilog(filename, mockturtle::verilog_reader( aig ));
            std::cout << "AIG network stored" << std::endl;
            store<mockturtle::aig_network>().extend() = aig;

            if(aig._storage->inputNames.size() == 0){
              for(int i = 0; i < aig.num_pis(); i++){
                std::string input_name = "pi";
                input_name.append(std::to_string(i));
                aig._storage->inputNames[i] = input_name;
              }
            }
            if(aig._storage->outputNames.size() == 0){
              for(int i = 0; i < aig.num_pos(); i++){
                std::string output_name = "po";
                output_name.append(std::to_string(i));
                aig._storage->outputNames[i] = output_name;
              }
            }

            filename.erase(filename.end() - 2, filename.end());
            aig._storage->net_name = filename;

            std::cout << "Verilog file written\n";
          }

        }
        else{
          std::cout << filename << " is not a valid Verilog file\n";
        }
        
      }
    private:
      std::string filename{};
    };

  ALICE_ADD_COMMAND(read_verilog, "Input");


  /*Reads an aig file and stores the AIG network in a store*/
  class read_aig_command : public alice::command{

    public:
      explicit read_aig_command( const environment::ptr& env )
          : command( env, "Uses the lorina library to read in an aig file" ){

        opts.add_option( "--filename,filename", filename, "AIG file to read in" )->required();
        add_flag("--mig,-m", "Store AIG file as MIG network (AIG network is default)");
      }

    protected:
      void execute(){

        if(checkExt(filename, "aig")){
          if(is_set("mig")){
            mockturtle::mig_network ntk;
            lorina::read_aiger(filename, mockturtle::aiger_reader( ntk ));
                
            store<mockturtle::mig_network>().extend() = ntk;


            if(ntk._storage->inputNames.size() == 0){
              for(int i = 0; i < ntk.num_pis(); i++){
                std::string input_name = "pi";
                input_name.append(std::to_string(i));
                ntk._storage->inputNames[i] = input_name;
              }
            }
            if(ntk._storage->outputNames.size() == 0){

              for(int i = 0; i < ntk.num_pos(); i++){
                std::string output_name = "po";
                output_name.append(std::to_string(i));
                ntk._storage->outputNames[i] = output_name;
              }
            }

            filename.erase(filename.end() - 4, filename.end());
            ntk._storage->net_name = filename;
          }
          else{
            mockturtle::aig_network ntk;
            lorina::read_aiger(filename, mockturtle::aiger_reader( ntk ));
                
            store<mockturtle::aig_network>().extend() = ntk;


            if(ntk._storage->inputNames.size() == 0){

              for(int i = 0; i < ntk.num_pis(); i++){
                std::string input_name = "pi";
                input_name.append(std::to_string(i));
                ntk._storage->inputNames[i] = input_name;
              }
            }
            if(ntk._storage->outputNames.size() == 0){

              for(int i = 0; i < ntk.num_pos(); i++){
                std::string output_name = "po";
                output_name.append(std::to_string(i));
                ntk._storage->outputNames[i] = output_name;
              }
            }

            filename.erase(filename.end() - 4, filename.end());
            ntk._storage->net_name = filename;
          }

        }
        else{
            std::cout << filename << " is not a valid aig file\n";
        }
        
      }
    private:
      std::string filename{};
    };

  ALICE_ADD_COMMAND(read_aig, "Input");

  class lut_map_command : public alice::command{

    public:
      explicit lut_map_command( const environment::ptr& env )
          : command( env, "Performs technology mapping of the network" ){

        opts.add_option( "--lut_size,-K", lut_size, "LUT size for mapping [DEFAULT = 6]" );
        opts.add_option( "--cut_size,-C", cut_size, "Max number of priority cuts [DEFAULT = 8]" );
        add_flag("--mig,-m", "Read from the stored MIG network");
        add_flag("--out,-o", "Write LUT mapping to bench file");
      }

    protected:
        void execute(){
            
          if(is_set("mig")){
            if(!store<mockturtle::mig_network>().empty()){
                auto& mig = store<mockturtle::mig_network>().current();
                std::string filename = mig._storage->net_name + "_lut.bench";
                mockturtle::mapping_view<mockturtle::mig_network, true> mapped{mig};

                mockturtle::lut_mapping_params ps;
                ps.cut_enumeration_ps.cut_size = lut_size;
                ps.cut_enumeration_ps.cut_limit = cut_size;

                mockturtle::lut_mapping<mockturtle::mapping_view<mockturtle::mig_network, true>, true>( mapped, ps );

                const auto klut_opt = mockturtle::collapse_mapped_network<mockturtle::klut_network>( mapped );
                auto const& klut = *klut_opt;

                mockturtle::depth_view klut_depth{klut};
                std::cout << "LUT = " << mapped.num_cells() << " lev = " << klut_depth.depth() << "\n";
                std::cout << "Finshed LUT mapping\n";
                if(is_set("out")){
                  std::cout << "filename = " << filename << "\n";
                  mockturtle::write_bench(klut, filename);
                }
            }
            else{
              std::cout << "There is not an MIG network stored.\n";
            }
          }
          else{
            if(!store<mockturtle::aig_network>().empty()){
              auto& aig = store<mockturtle::aig_network>().current();
              std::string filename = aig._storage->net_name + "_lut.bench";
              mockturtle::mapping_view<mockturtle::aig_network, true> mapped{aig};

              mockturtle::lut_mapping_params ps;
              ps.cut_enumeration_ps.cut_size = lut_size;
              ps.cut_enumeration_ps.cut_limit = cut_size;

              mockturtle::lut_mapping<mockturtle::mapping_view<mockturtle::aig_network, true>, true>( mapped, ps );

              const auto klut_opt = mockturtle::collapse_mapped_network<mockturtle::klut_network>( mapped );
              auto const& klut = *klut_opt;

              mockturtle::depth_view klut_depth{klut};
              std::cout << "LUT = " << mapped.num_cells() << " lev = " << klut_depth.depth() << "\n";
              std::cout << "Finshed LUT mapping\n";
              if(is_set("out")){
                std::cout << "filename = " << filename << "\n";
                mockturtle::write_bench(klut, filename);
              }
            }
            else{
              std::cout << "There is not an AIG network stored.\n";
            }
          }
        }    
    private:
      int lut_size = 6;
      int cut_size = 8;
    };

  ALICE_ADD_COMMAND(lut_map, "LUT");


  ALICE_COMMAND( cut_e, "Optimization", "Performs Cut Enumeration on a stored AIG network"){
  	auto aig = store<mockturtle::aig_network>().current();
  	const mockturtle::network_cuts cuts = cut_enumeration( aig );
  	//store<mockturtle::network_cuts>().extend() = cuts;
  	aig.foreach_node( [&]( auto node ) {
  			std::cout << cuts.cuts( aig.node_to_index( node ) ) << "\n";
  	} );
  }

  class optimization_command : public alice::command{

    public:
        explicit optimization_command( const environment::ptr& env )
                : command( env, "Classify partitions and perform corresponding optimization" ){

            opts.add_option( "--cnn_model,-c", cnn_model, "Trained CNN model for classification" );
            opts.add_option( "--out,-o", out_file, "Verilog output" )->required();
            add_flag("--brute,-b", "Uses a brute force approach instead of classification");
            add_flag("--aig,-a", "Perform only AIG optimization on all partitions");
            add_flag("--mig,-m", "Perform only MIG optimization on all partitions");
        }

    protected:
      void execute(){

        mockturtle::mig_npn_resynthesis resyn_mig;
        mockturtle::xag_npn_resynthesis<mockturtle::aig_network> resyn_aig;
        std::vector<int> aig_parts;
        std::vector<int> mig_parts;
        if(!store<mockturtle::aig_network>().empty()){

          auto ntk_aig = store<mockturtle::aig_network>().current();
          std::string file_base = ntk_aig._storage->net_name;
          std::cout << "ntk_aig size = " << ntk_aig.size() << "\n";
          std::string net_name = ntk_aig._storage->net_name;

          if(!store<oracle::partition_manager<mockturtle::aig_network>>().empty()){

            auto partitions_aig = store<oracle::partition_manager<mockturtle::aig_network>>().current();
            int num_parts = partitions_aig.get_part_num();
            if(is_set("aig")){
              for(int i = 0; i < num_parts; i++){
                aig_parts.push_back(i);
              }
            }
            else if(is_set("mig")){
              for(int i = 0; i < num_parts; i++){
                mig_parts.push_back(i);
              }
            }
            else if(is_set("brute")){

              for(int i = 0; i < num_parts; i++){
                oracle::partition_view<mockturtle::aig_network> part_aig = partitions_aig.create_part(ntk_aig, i);

                auto opt_aig = mockturtle::node_resynthesis<mockturtle::aig_network>( part_aig, resyn_aig );
                mockturtle::depth_view part_aig_depth{opt_aig};
                std::cout << "aig part size = " << opt_aig.num_gates() << " and depth = " << part_aig_depth.depth() << "\n";
                mockturtle::aig_script aigopt;
                opt_aig = aigopt.run(opt_aig);
                mockturtle::depth_view part_aig_opt_depth{opt_aig};
                int aig_opt_size = opt_aig.num_gates();
                int aig_opt_depth = part_aig_opt_depth.depth();
                std::cout << "optimized aig part size = " << aig_opt_size << " and depth = " << aig_opt_depth << "\n";

                auto opt_mig = mockturtle::node_resynthesis<mockturtle::mig_network>( part_aig, resyn_mig );
                mockturtle::depth_view part_mig_depth{opt_mig};
                std::cout << "mig part size = " << opt_mig.num_gates() << " and depth = " << part_mig_depth.depth() << "\n";
                mockturtle::mig_script migopt;
                opt_mig = migopt.run(opt_mig);
                mockturtle::depth_view part_mig_opt_depth{opt_mig};
                int mig_opt_size = opt_mig.num_gates();
                int mig_opt_depth = part_mig_opt_depth.depth();
                std::cout << "optimized mig part size = " << mig_opt_size << " and depth = " << mig_opt_depth << "\n";

                if((aig_opt_size * aig_opt_depth) <= (mig_opt_size * mig_opt_depth)){
                  std::cout << "AIG wins\n";
                  aig_parts.push_back(i);
                }
                else{
                  std::cout << "MIG wins\n";
                  mig_parts.push_back(i);
                }
              }

            }
            else{
              if(!cnn_model.empty()){
                partitions_aig.run_classification(ntk_aig, cnn_model);

                aig_parts = partitions_aig.get_aig_parts();
                mig_parts = partitions_aig.get_mig_parts();
              }
              else{
                std::cout << "Must include CNN model json file\n";
              }

            }
            
            mockturtle::mig_network ntk_mig = aig_to_mig(ntk_aig);
            oracle::partition_manager<mockturtle::mig_network> partitions_mig(ntk_mig, partitions_aig.get_all_part_connections(), 
                    partitions_aig.get_all_partition_inputs(), partitions_aig.get_all_partition_outputs(), partitions_aig.get_part_num());
            
            std::cout << "AIG Optimization\n";
            for(int i = 0; i < aig_parts.size(); i++){
              std::cout << "Optimize partition " << aig_parts.at(i) << "\n";
              oracle::partition_view<mockturtle::mig_network> part = partitions_mig.create_part(ntk_mig, aig_parts.at(i));
              mockturtle::depth_view part_depth{part};
              std::cout << "part size = " << part.num_gates() << " and depth = " << part_depth.depth() << "\n";

              auto opt = mockturtle::node_resynthesis<mockturtle::aig_network>( part, resyn_aig );

              mockturtle::aig_script aigopt;
              opt = aigopt.run(opt);

              auto opt_mig = aig_to_mig(opt);
              mockturtle::depth_view part_opt_depth{opt_mig};
              std::cout << "new part size = " << opt_mig.num_gates() << " and depth = " << part_opt_depth.depth() << "\n";

              partitions_mig.synchronize_part(part, opt_mig, ntk_mig);
            }
            std::cout << "MIG Optimization\n";
            for(int i = 0; i < mig_parts.size(); i++){
              std::cout << "Optimize partition " << mig_parts.at(i) << "\n";
              oracle::partition_view<mockturtle::mig_network> part = partitions_mig.create_part(ntk_mig, mig_parts.at(i));
              mockturtle::depth_view part_depth{part};
              std::cout << "part size = " << part.num_gates() << " and depth = " << part_depth.depth() << "\n";

              auto opt = mockturtle::node_resynthesis<mockturtle::mig_network>( part, resyn_mig );
              
              mockturtle::mig_script migopt;
              opt = migopt.run(opt);
              
              mockturtle::depth_view part_opt_depth{opt};
              std::cout << "new part size = " << opt.num_gates() << " and depth = " << part_opt_depth.depth() << "\n";

              partitions_mig.synchronize_part(part, opt, ntk_mig);
            }
            std::cout << aig_parts.size() << " AIGs and " << mig_parts.size() << " MIGs\n";
            std::cout << "AIG partitions = {";
            for(int i = 0; i < aig_parts.size(); i++){
              std::cout << aig_parts.at(i) << " ";
            }
            std::cout << "}\n";
            std::cout << "MIG partitions = {";
            for(int i = 0; i < mig_parts.size(); i++){
              std::cout << mig_parts.at(i) << " ";
            }
            std::cout << "}\n";
            
            partitions_mig.connect_outputs(ntk_mig);
            
            mockturtle::depth_view ntk_before_depth2{ntk_mig};
            
            ntk_mig = mockturtle::cleanup_dangling( ntk_mig );
            mockturtle::depth_view ntk_depth2{ntk_mig};
            std::cout << "Final ntk size = " << ntk_mig.num_gates() << " and depth = " << ntk_depth2.depth() << "\n";
            store<mockturtle::mig_network>().extend() = ntk_mig;
            mockturtle::write_verilog(ntk_mig, out_file);
        
          }
          else{
              std::cout << "AIG not partitioned yet\n";
          }
        }
        else{
          std::cout << "No AIG stored\n";
        }
      }
    private:
        std::string cnn_model{};
        std::string out_file{};
    };

  ALICE_ADD_COMMAND(optimization, "Optimization");


  ALICE_COMMAND( partition_sizes, "Partitioning", "Shows the size of each partition"){

    if(!store<mockturtle::aig_network>().empty()){
      auto aig = store<mockturtle::aig_network>().current();
      for(int i = 0; i < aig._storage->num_partitions; i++){
        std::cout << "Partition=" << i << ": ";
        std::cout << aig._storage->partitionSize[i] << "\n";
      }
    }
    else{
      std::cout << "There is no AIG network stored\n";
    }
  }

  ALICE_COMMAND( read_lib, "STA", "Reads standard cell library"){
    std::string filename = "";
    std::cout << "Enter liberty path: ";
    std::cin >> filename;

    sta_cfg.set_lib_path(filename);
  }

  ALICE_COMMAND( read_netlist, "STA", "Reads mapped verilog"){
    std::string filename = "";
    std::cout << "Enter verilog path: ";
    std::cin >> filename;

    sta_cfg.set_netlist_path(filename);
  }

  ALICE_COMMAND( read_sdc, "STA", "Reads constraint file"){
    std::string filename = "";
    std::cout << "Enter sdc path: ";
    std::cin >> filename;

    sta_cfg.set_sdc_path(filename);
  }

  ALICE_COMMAND( run_slack, "STA", "Shows WNS and TNS"){

    sta_cfg.run_slack();
  }

  ALICE_COMMAND( report_critical_path, "STA", "Runs STA"){

    sta_cfg.run_report_timing();
  }

  ALICE_COMMAND(show_mig, "Output", "Writes the the mig dot file"){
  	auto mig = store<mockturtle::mig_network>().current();

  	for (int j =1; j < mig._storage->nodes.size(); j++) {
  		for (int i = 0; i < mig._storage->nodes.data()->children.size(); i++) {
  			std::cout << "node index " << j << " node fan in " << mig._storage->nodes[j].children[i].data << std::endl;
  		}
  	}
  	for (unsigned k = mig.num_pis()+1; k<= mig._storage->inputs.size(); k++ ){
  		auto node = mig.index_to_node(k);
  		std::cout << " reg " << k << " fan out size " << mig.fanout_size(node) << std::endl;
    }
    for (unsigned l=0; l< mig._storage->outputs.size(); l++){
      std::cout << " outputs " << std::endl;
      std::cout << " node fan in data " << mig._storage->outputs[l].data << std::endl;
    }
  }

  ALICE_COMMAND(show_aig, "Output", "Writes the the aig dot file"){

      auto aig = store<mockturtle::aig_network>().current();

    for (int j =1; j < aig._storage->nodes.size(); j++) {
      for (int i = 0; i < aig._storage->nodes.data()->children.size(); i++) {
        std::cout << "node index " << j << " node fan in " << aig._storage->nodes[j].children[i].data << std::endl;
      }
    }

    for (unsigned k = aig.num_pis()+1; k<= (aig._storage->inputs.size() - aig.num_latches()); k++ ){
    	auto node = aig.index_to_node(k);
	    std::cout << " reg " << k << " fan out size " << aig.fanout_size(node) << std::endl;
    }

  	for( int outIndex=0; outIndex<aig.num_pos()- aig.num_latches(); outIndex++){
  		std::cout << "Output " << outIndex << " data " << aig._storage->outputs[outIndex].data << std::endl;
  	}

  	aig.foreach_ri([&](auto fi) {
  		std::cout << "Register " << aig.ri_index(fi) << " data " << fi.data << std::endl;
  	});
  }

  ALICE_COMMAND(print_eachout, "Output", "Testing foreach") {
  	auto aig = store<mockturtle::aig_network>().current();
  	static int counter = 0;
  	aig.foreach_po([&](auto fi) {
  		std::cout << "Out index " << aig.po_index(fi) << " Counter " << counter << std::endl;
  	});
  	counter++;
  }

  ALICE_COMMAND(depth, "Output", "Testing depth") {
  	auto aig = store<mockturtle::aig_network>().current();

  	mockturtle::depth_view aig_depth{aig};

  	std::cout << "Aig level " << aig_depth.depth()  << std::endl;
  }

  ALICE_COMMAND(get_cones, "Output", "Get logical cones for each PO") {

  	auto aig = store<mockturtle::aig_network>().current();

  	//map with number of nodes in each logical cone
  	std::unordered_map<int, int> po_nodes;
  	std::unordered_map<int, int> ri_nodes;

  	//number of inputs for each cone
  	std::unordered_map<int, int> po_ins;
  	std::unordered_map<int, int> ri_ins;

  	//first processing logical cones for POs
  	for(int outIndex=0; outIndex<aig.num_pos()- aig.num_latches(); outIndex++) {

  		aig.foreach_node([&](auto node) {
  			//set all nodes as not visited
  			aig._storage->nodes[node].data[1].h1 = 0;
  		});

  		//start counter for a given output index
  		po_nodes.insert(std::make_pair(outIndex, 0));

  		//starting the counter of inputs
  		po_ins.insert(std::make_pair(outIndex, 0));

  		//calculate the index of the node driving the output
  		auto inIdx = aig._storage->outputs[outIndex].data;
  		if (aig._storage->outputs[outIndex].data & 1)
  			inIdx = aig._storage->outputs[outIndex].data - 1;

  		inIdx = inIdx >> 1;

  		//call DFS
  		compute_cone(aig, inIdx, po_nodes, outIndex, po_ins);

  		aig.foreach_node([&](auto node) {
  			//set all nodes as not visited
  			aig._storage->nodes[node].data[1].h1 = 0;
  		});

  		int level = computeLevel(aig, inIdx);
  		int nodes = 0;
  		int inputs = 0;

  		// for each output prints index, nodes, depth and number of inputs, respectively
  		std::unordered_map<int, int>::iterator it;
  		it = po_nodes.find(outIndex);

  		if (it != po_nodes.end())
  			nodes = it->second;

  		std::unordered_map<int, int>::iterator init;
  		init = po_ins.find(outIndex);

  		if (it != po_nodes.end())
  			inputs = init->second;

  		std::cout << "Output " << outIndex << " " << nodes << " " << level << " " << inputs << std::endl;
  		
  	}

  	//processing logical cones for registers - pseudo POs
  	aig.foreach_ri([&](auto fi) {
  		aig.foreach_node([&](auto node) {
  			//set all nodes as not visited
  			aig._storage->nodes[node].data[1].h1 = 0;
  		});

            //PO index to keep track of each cone on the map structure
  		auto outIndex = aig.ri_index(fi);

  		//start counter for a given ri index
  		ri_nodes.insert(std::make_pair(outIndex, 0));
  		ri_ins.insert(std::make_pair(outIndex, 0));

  		//calculate the index of the node driving the output
  		auto inIndex = fi.data;
  		if(fi.data & 1)
  			inIndex = fi.data - 1;

  		//calculate latch input node index
  		inIndex = inIndex >> 1;

  		//call DFS
      compute_cone(aig, inIndex, ri_nodes, outIndex, ri_ins);

      aig.foreach_node([&](auto node) {
          //set all nodes as not visited
          aig._storage->nodes[node].data[1].h1 = 0;
      });

      int level = computeLevel(aig, inIndex);
      int nodes = 0;
  		int inputs = 0;

  		// for each output prints index, nodes, depth and number of inputs, respectively
  		std::unordered_map<int, int>::iterator it;
  		it = ri_nodes.find(outIndex);

  		if(it != po_nodes.end())
  			nodes = it->second;

  		std::unordered_map<int, int>::iterator init;
  		init = ri_ins.find(outIndex);

  		if(it != po_nodes.end())
  			inputs = init->second;

  		std::cout << "Register " << outIndex << " " << nodes << " " << level << " " << inputs << std::endl;
  	});
  }

  ALICE_COMMAND(disjoint_clouds, "Output", "Writes the status for each disjoint combinational cloud on the current AIG"){
  	std::string filename = " ";
  	std::cout << "Enter filename: ";
  	std::cin >> filename;

  	std::ofstream dset;
  	dset.open (filename);

  	auto aig = store<mockturtle::aig_network>().current();

  	//maps to keep the depth and numb of nodes for each comb cloud
  	std::unordered_map<int, int> outputs;
  	std::unordered_map<int, int> depth;
  	std::unordered_map<int, int> number_nodes;


  	//copy sets in a local vector
  	std::vector<int> clouds;

    //number of aig inputs + 1 to start at the nodes index
    uint64_t inSize = aig.num_pis()+1;

    //union find constructor - inSize sets the vector index where and nodes start
    UnionFind uf(aig.size()-1, inSize);

  	//set all nodes as not visited for the DFS
  	aig.foreach_node([&] (auto node) {
      aig._storage->nodes[node].data[1].h1 = 0;
    });

    //first looking to latches
    aig.foreach_ri( [&] (auto ri) {

      auto indata = ri.data;

      if(indata & 1)
        indata = indata - 1;

      //calculate latch input node index
      auto inIndex = indata >> 1;

      if(inIndex>aig.num_pis()) {
        dfs(aig, inIndex, uf);
      }
    });

  	aig.foreach_node([&] (auto node) {
  		//set all nodes as not visited
  		aig._storage->nodes[node].data[1].h1 = 0;
  	});

    //then considering POs
    for(int i = 0; i<aig.num_pos()-aig.num_latches(); i++){
  		auto inIdx = aig._storage->outputs[i].data;
  		if(aig._storage->outputs[i].data & 1)
  			inIdx = aig._storage->outputs[i].data - 1;
  		//calculate the index of the node driving the output
  		inIdx = inIdx >> 1;
  		if(inIdx>aig.num_pis()) {
  			dfs(aig, inIdx, uf);
  		}
    }

    uf.get_sets(clouds);

    for (int j = 0; j < clouds.size() ; ++j) {
  		auto it = number_nodes.find (clouds[j]);
  		//element not in the table yet
  		if(it==number_nodes.end()){
  			int count = std::count(std::begin(clouds), std::end(clouds), clouds[j]);
  			number_nodes.insert(std::make_pair(clouds[j],count));
  		}
  	}

  	for (std::pair<int, int> element : number_nodes) {
  		dset << element.second << std::endl;
  	}

  	std::cout << "Number of disjoint graphs: " << uf.getNumbOfSets() << std::endl;

  	dset.close();
  }

  class write_verilog_command : public alice::command{

    public:
      explicit write_verilog_command( const environment::ptr& env )
          : command( env, "Writes the Boolean network into structural verilog" ){

        opts.add_option( "--filename,filename", filename, "Verilog file to write out to" )->required();
        add_flag("--mig,-m", "Read from the MIG network");
      }

    protected:
        void execute(){
        if(checkExt(filename, "v")){
          if(is_set("mig")){
            if(!store<mockturtle::mig_network>().empty()){
              auto& mig = store<mockturtle::mig_network>().current();
              mockturtle::write_verilog(mig, filename);
            }
            else{
              std::cout << "There is not an MIG network stored.\n";
            }
          }
          else{
            if(!store<mockturtle::aig_network>().empty()){
              auto& aig = store<mockturtle::aig_network>().current();
              mockturtle::write_verilog(aig, filename);
            }
            else{
              std::cout << "There is not an AIG network stored.\n";
            }
          }
        }
        else{
            std::cout << filename << " is not a valid verilog file\n";
        }
      }
    private:
      std::string filename{};
  };

  ALICE_ADD_COMMAND(write_verilog, "Output");

  class write_bench_command : public alice::command{

    public:
      explicit write_bench_command( const environment::ptr& env )
          : command( env, "Writes the Boolean network into bench format" ){

        opts.add_option( "--filename,filename", filename, "Bench file to write out to" )->required();
        add_flag("--mig,-m", "Read from the MIG network");
      }

    protected:
      void execute(){
        if(checkExt(filename, "bench")){
          if(is_set("mig")){
            if(!store<mockturtle::mig_network>().empty()){
              auto& mig = store<mockturtle::mig_network>().current();
              mockturtle::write_bench(mig, filename);
            }
            else{
              std::cout << "There is not an MIG network stored.\n";
            }
          }
          else{
            if(!store<mockturtle::aig_network>().empty()){
              auto& aig = store<mockturtle::aig_network>().current();
              mockturtle::write_bench(aig, filename);
            }
            else{
              std::cout << "There is not an AIG network stored.\n";
            }
          }
        }
        else{
            std::cout << filename << " is not a valid bench file\n";
        }
      }
    private:
      std::string filename{};
  };

  ALICE_ADD_COMMAND(write_bench, "Output");

  class write_dot_command : public alice::command{

    public:
      explicit write_dot_command( const environment::ptr& env )
          : command( env, "Writes the Boolean network into structural verilog" ){

        opts.add_option( "--filename,filename", filename, "Verilog file to write out to" )->required();
        add_flag("--mig,-m", "Read from the MIG network and MIG partition manager for truth table generation");
      }

    protected:
      void execute(){
        if(checkExt(filename, "dot")){
          if(is_set("mig")){
            if(!store<mockturtle::mig_network>().empty()){
              auto& mig = store<mockturtle::mig_network>().current();
              mockturtle::write_dot(mig, filename);
            }
            else{
              std::cout << "There is not an MIG network stored.\n";
            }
          }
          else{
            if(!store<mockturtle::aig_network>().empty()){
              auto& aig = store<mockturtle::aig_network>().current();
              mockturtle::write_dot(aig, filename);
            }
            else{
              std::cout << "There is not an AIG network stored.\n";
            }
          }
            
        }
        else{
            std::cout << filename << " is not a valid dot file\n";
        }
      }

    private:
      std::string filename{};
    };

  ALICE_ADD_COMMAND(write_dot, "Output");

  ALICE_COMMAND(tmap, "Transformation", "Performs LUT techmapping") {
    auto mig = store<mockturtle::mig_network>().current();;
    mockturtle::mapping_view<mockturtle::mig_network, true> mapped_mig{mig};

    mockturtle::lut_mapping_params ps;
    ps.cut_enumeration_ps.cut_size = 6;
    mockturtle::lut_mapping(mapped_mig, ps);
  }

  ALICE_COMMAND(interleaving, "Modification", "NPN + depth MIG rewriting") {
    auto& mig = store<mockturtle::mig_network>().current();

    mockturtle::mig_npn_resynthesis resyn;
    mockturtle::cut_rewriting_params ps;

    ps.cut_enumeration_ps.cut_size = 4;

    mockturtle::cut_rewriting(mig, resyn, ps);
    mig = mockturtle::cleanup_dangling( mig );

  }

  ALICE_COMMAND(migscript, "Modification", "Exact NPN MIG rewriting") {
  	auto& opt = store<mockturtle::mig_network>().current();
    mockturtle::depth_view mig_depth{opt};

  	//DEPTH REWRITING
    std::cout << "MIG logic depth " << mig_depth.depth() << " Majority nodes " << opt.num_gates() << std::endl;

    mockturtle::mig_script migopt;
    opt = migopt.run(opt);

    mockturtle::depth_view new_mig_depth{opt};
    std::cout << "MIG logic depth " << new_mig_depth.depth() << " Majority nodes " << opt.num_gates() << std::endl;

  }

  ALICE_COMMAND(aigscript, "Modification", "NPN XAG cut rewriting") {
    auto& opt = store<mockturtle::aig_network>().current();
    mockturtle::depth_view aig_depth{opt};

    //DEPTH REWRITING
    std::cout << "AIG logic depth " << aig_depth.depth() << " nodes " << opt.num_gates() << std::endl;

    mockturtle::aig_script aigopt;
    opt = aigopt.run(opt);

    mockturtle::depth_view new_aig_depth{opt};
    std::cout << "AIG logic depth " << new_aig_depth.depth() << " nodes " << opt.num_gates() << std::endl;

  }

  ALICE_COMMAND(depthr, "Modification", "Logic depth oriented MIG rewriting"){
    auto& mig = store<mockturtle::mig_network>().current();
  	std::cout << "Mig gates " << mig.num_gates() << std::endl;

  	//to compute at level
  	mockturtle::depth_view mig_depth{mig};

  	std::cout << "Mig level " << mig_depth.depth() << " mig gates " << mig.num_gates() << std::endl;

  	mockturtle::mig_algebraic_depth_rewriting_params pm;
  	pm.aggressive;
  	mockturtle::mig_algebraic_depth_rewriting(mig_depth, pm);

  	mig = mockturtle::cleanup_dangling( mig );
  	std::cout << "Mig level after algebraic rewriting " << mig_depth.depth() << " Mig gates " << mig.num_gates() << std::endl;
  }

  ALICE_COMMAND(test_ci, "Output", "Testing mig is ci") {
  	auto mig = store<mockturtle::mig_network>().current();

  	mig.foreach_ci([&] (auto node) {
  		if(mig.is_ci(node)){
  			std::cout << "Found CI " << std::endl;
  		}
  	});
  }

  ALICE_CONVERT( mockturtle::mig_network, element, mockturtle::aig_network ) {
    auto aig = mockturtle::aig_network();
  	std::unordered_set<uint32_t> toinvert;
  	std::unordered_set<uint32_t> duplicated;
  	std::map<uint32_t , mockturtle::aig_network::signal> update_output ;
  	std::vector<int> missing;

    //create aig inputs
  	for (int i = 0; i < element.num_pis() - element.num_latches(); ++i) {
  		aig.create_pi();
  	}

    //create registers output (PIs in the AIG)
    for (int i=0; i<element.num_latches(); i++){
    	aig.create_ro();
    }

    //convert majs into ands
  	element.foreach_gate([&] (auto gate) {
  		auto index = element.node_to_index(gate);

  		//ignore PIs and Latch inputs
  		if(index > element.num_pis()) {

  			if(element._storage->nodes[index].children[0].data == 1){

  				auto child2 = element._storage->nodes[index].children[1];
  				auto child3 = element._storage->nodes[index].children[2];

  				auto search = toinvert.find(2 * child2.index);
  				if (search == toinvert.end()) {
  					child2 = aig.create_not(child2);
  				}

  				auto search1 = toinvert.find(2 * child3.index);
  				if (search1 == toinvert.end()) {
  					child3 = aig.create_not(child3);
  				}

  				//add complemented nodes. If a complemented node (first child equals to 1) is fan-in of a node, it should be complemented while generating the aig
  				toinvert.insert(2 * index);

  				aig.create_maj(aig.child_to_signal(0), child2, child3);
  			}

  			else if (element._storage->nodes[index].children[0].data == 0){
  				auto child2 = element._storage->nodes[index].children[1];
  				auto child3 = element._storage->nodes[index].children[2];

  				auto search = toinvert.find(2 * child2.index);
  				if (search != toinvert.end()) {
  					child2 = aig.create_not(child2);
  					duplicated.insert(child2.index * 2);
  					toinvert.erase(2*index);
  				}

  				auto search1 = toinvert.find(2 * child3.index);
  				if (search1 != toinvert.end()) {
  					child3 = aig.create_not(child3);
  					duplicated.insert(child3.index * 2);
  				}

  				aig.create_maj(aig.child_to_signal(0), child2, child3);
  			}

  			else{
  				auto child1 = element._storage->nodes[index].children[0];
  				auto child2 = element._storage->nodes[index].children[1];
  				auto child3 = element._storage->nodes[index].children[2];

  				auto search = toinvert.find(2 * child1.index);
  				auto search0 = duplicated.find(2 * child1.index);
  				if (search != toinvert.end() && search0 == duplicated.end()) {
  					child1 = aig.create_not(child1);
  				}

  				auto search1 = duplicated.find(2 * child2.index);
  				auto search2 = toinvert.find(2 * child2.index);
  				if (search2 != toinvert.end() && search1 == duplicated.end()) {
  					child2 = aig.create_not(child2);
  				}

  				auto search3 = toinvert.find(2 * child3.index);
  				auto search4 = duplicated.find(2*child3.index);
  				if (search3 != toinvert.end() && search4 == duplicated.end()) {
  					child3 = aig.create_not(child3);
  				}

  				auto maj = aig.create_maj(child1, child2, child3);

  				std::cout << "Inserting in the table " << 2*index << " " << maj.index << std::endl;
  				update_output.insert(std::make_pair(2*index, maj));

  			}
  		}
  	});

  	aig.foreach_gate([&] (auto gate) {
  		auto index = aig.node_to_index(gate);

  		//ignore PIs and Latch inputs
  		if(index > aig.num_pis()) {

  			auto child1 = aig._storage->nodes[index].children[0];
  			auto child2 = aig._storage->nodes[index].children[1];

  			auto it = update_output.find(2*child1.index);

  			if(it!=update_output.end()){
  				if(index < (it->first)/2) {
  					//it->second.complement = it->second.complement ^ 1;
  					std::cout << "Child1! My index " << index <<std::endl;
  					std::cout << "Replacing " << 2*child1.index << " by " << it->second.index << " polarity " << it->second.complement << std::endl;
  					aig._storage->nodes[index].children[0] = it->second;
  					//aig.update();
  				}
  			}

  			auto it1 = update_output.find(2*child2.index);

  			if(it1!=update_output.end()){
  				if(index < (it1->first)/2) {
  					//it1->second.complement = it1->second.complement ^ 1;
  					std::cout << "Child2! My index " << index <<std::endl;
  					std::cout << "Replacing " << 2*child2.index << " by " << it1->second.index << " polarity " << it1->second.complement << std::endl;
  					aig._storage->nodes[index].children[1] = it1->second;
  					//aig.update();
  				}
  			}
  		}
  	});

    //create aig outputs
  	for(int i = 0; i < element.num_pos() - element.num_latches(); i++){
  		auto in = element._storage->outputs[i].data;
  		auto signal = element.child_to_signal(in);
  		auto signalOut = mockturtle::aig_network::signal(signal);

  		auto it = update_output.find(in);

  		if(it!=update_output.end()){
  			//std::cout << "Need to replace node " << in << " by " << it->second.index << std::endl;
  			signalOut = it->second;
  		}

  		else {
  			auto search = toinvert.find(1 ^ in);
  			if (search != toinvert.end()) {
  				signalOut = aig.create_not(signalOut);
  			}

  			auto search1 = toinvert.find(in);
  			if (search1 != toinvert.end()) {
  				signalOut = aig.create_not(signalOut);
  			}
  		}
  		aig.create_po(signalOut);
  	}

  	//create registers input as AIG PO
  	for (unsigned int i = 0; i < element.num_latches() ; ++i) {
  		int8_t reset = 0;
  		std::string name = "";
  		auto regIdx = element.num_pos() - element.num_latches() + i;
  		auto signal = element.ri_at(regIdx);

  		bool comp = false;
  		auto temp = signal.data;

  		auto it = update_output.find(2*signal.index);

  		if(it!=update_output.end()){
  			//std::cout << "Need to replace node " << in << " by " << it->second.index << std::endl;
  			auto ri = it->second;
  			aig.create_ri(ri, reset, name);

  		}

  		else{
  			if (signal.data & 1) {
  				auto temp = signal.data - 1;
  			}

  			auto nodeIn = signal.data / 2;
  			if (element._storage->nodes[nodeIn].children[0].data == 1) {
  				comp = true;
  			}

  			auto ri = mockturtle::aig_network::signal(signal);

  			if (comp) {
  				ri = aig.create_not(ri);
  			}

  			aig.create_ri(ri, reset, name);
  		}
  	}
    return aig;
  }
} // namespace alice

/* Main method for the Alice shell (with prefix) */
ALICE_MAIN( lsoracle )