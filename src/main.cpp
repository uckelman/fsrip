/*
src/main.cpp

Created by Jon Stewart on 2010-01-04.
Copyright (c) 2010 Lightbox Technologies, Inc.
*/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <iostream>

#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>

#include "walkers.h"

namespace po = boost::program_options;

void printHelp(const po::options_description& desc) {
  std::cout << "fsrip, Copyright (c) 2010-2012, Lightbox Technologies, Inc." << std::endl;
  std::cout << "Built " << __DATE__ << std::endl;
  std::cout << "TSK version is " << tsk_version_get_str() << std::endl;
  std::cout << desc << std::endl;
}

std::shared_ptr<LbtTskAuto> createVisitor(const std::string& cmd, std::ostream& out, const std::vector<std::string>& segments) {
  if (cmd == "dumpimg") {
    return std::shared_ptr<LbtTskAuto>(new ImageDumper(out));
  }
  else if (cmd == "dumpfs") {
    return std::shared_ptr<LbtTskAuto>(new MetadataWriter(out));
  }
  else if (cmd == "count") {
    return std::shared_ptr<LbtTskAuto>(new FileCounter(out));
  }
  else if (cmd == "info") {
    return std::shared_ptr<LbtTskAuto>(new ImageInfo(out, segments));
  }
  else if (cmd == "dumpfiles") {
    return std::shared_ptr<LbtTskAuto>(new FileWriter(out));
  }
  else {
    return std::shared_ptr<LbtTskAuto>();
  }
}

int main(int argc, char *argv[]) {
  std::string ucMode;

  po::options_description desc("Allowed Options");
  po::positional_options_description posOpts;
  posOpts.add("command", 1);
  posOpts.add("ev-files", -1);
  desc.add_options()
    ("help", "produce help message")
    ("command", po::value< std::string >(), "command to perform [info|dumpimg|dumpfs|dumpfiles|count]")
    ("unallocated", po::value< std::string >(&ucMode)->default_value("none"), "how to handle unallocated [none|fragment|block]")
    ("ev-files", po::value< std::vector< std::string > >(), "evidence files");

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(posOpts).run(), vm);
    po::notify(vm);

    std::shared_ptr<LbtTskAuto> walker;

    std::vector< std::string > imgSegs;
    if (vm.count("ev-files")) {
      imgSegs = vm["ev-files"].as< std::vector< std::string > >();
    }
    if (vm.count("help")) {
      printHelp(desc);
    }
    else if (vm.count("command") && vm.count("ev-files") && (walker = createVisitor(vm["command"].as<std::string>(), std::cout, imgSegs))) {
      boost::scoped_array< TSK_TCHAR* >  segments(new TSK_TCHAR*[imgSegs.size()]);
      for (unsigned int i = 0; i < imgSegs.size(); ++i) {
        segments[i] = (TSK_TCHAR*)imgSegs[i].c_str();
      }
      if (0 == walker->openImage(imgSegs.size(), segments.get(), TSK_IMG_TYPE_DETECT, 0)) {
        walker->setFileFilterFlags(TSK_FS_DIR_WALK_FLAG_NOORPHAN);
        if (ucMode == "fragment") {
          walker->setUnallocatedMode(LbtTskAuto::FRAGMENT);
        }
        else if (ucMode == "block") {
          walker->setUnallocatedMode(LbtTskAuto::BLOCK);
        }
        else {
          walker->setUnallocatedMode(LbtTskAuto::NONE);
        }
        if (0 == walker->start()) {
          walker->startUnallocated();
          walker->finishWalk();
          return 0;
        }
        else {
          std::cout.flush();
          std::cerr << "Had an error parsing filesystem" << std::endl;
          for (auto& err: walker->getErrorList()) {
            std::cerr << err.msg1 << " " << err.msg2 << std::endl;
          }
        }
      }
      else {
        std::cerr << "Had an error opening the evidence file" << std::endl;
        return 1;
      }
    }
    else {
      std::cerr << "Error: did not understand arguments\n\n";
      printHelp(desc);
      return 1;
    }
  }
  catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << "\n\n";
    printHelp(desc);
    return 1;
  }
  return 0;
}
