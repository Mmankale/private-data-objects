#!groovy


node('master') {



    currentBuild.result = "SUCCESS"


           stage("Check for Signed-Off Commits") {
	                   sh '''#!/bin/bash -l
			                       if [ -v CHANGE_URL ] ;
					                           then
								                           temp_url="$(echo $CHANGE_URL |sed s#github.com/#api.github.com/repos/#)/commits"
											                           pull_url="$(echo $temp_url |sed s#pull#pulls#)"
														                           IFS=$'\n'
																	                           for m in $(curl -s "$pull_url" | grep "message") ; do
																				                               if echo "$m" | grep -qi signed-off-by:
																							                                   then
																											                                 continue
																															                             else
																																		                                   echo "FAIL: Missing Signed-Off Field"
																																						                                 echo "$m"
																																										                               exit 1
																																													                                   fi
																																																	                           done
																																																				                           unset IFS;
																																																							                       fi
																																																									                       '''
																																																											                   }

																																																													          stage('Build Docker'){

																																																														              sh '''#!/bin/bash -l
																																																															                          whoami
																																																																		                      sudo docker build --build-arg UBUNTU_VERSION=bionic --build-arg https_proxy=$https_proxy --build-arg http_proxy=$http_proxy --build-arg ftp_proxy=$ftp_proxy -f docker/Dockerfile.pdo-dev -t pdo-dev .
																																																																				                          cd /var/lib/jenkins/workspace/tcf_master
																																																																							                      sudo docker run --device=/dev/isgx -v /var/run/aesmd:/var/run/aesmd pdo-dev /bin/bash
																																																																									                          . /opt/intel/sgxsdk/environment
																																																																												                      export VIRTUAL_ENV=`pwd`/__tools__/build/_dev
																																																																														                          export CONTRACTHOME=`pwd`/__tools__/build/_dev/opt/pdo
																																																																																	                      export SGX_DEBUG=1
																																																																																			                          export SGX_MODE=SIM
																																																																																						                      #. `pwd`/__tools__/build/_dev/bin/activate
																																																																																								                          cd __tools__/
																																																																																											                      cd build/
																																																																																													                          make clean && make

																																																																																																                  '''
																																																																																																		         }
																																																																																																			 }
