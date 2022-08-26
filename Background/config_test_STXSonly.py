

backgroundScriptCfg = {
  
  # Setup
  'inputWSDir':'/eos/user/r/rsaradhy/vh_anomalous/2022_08_23_STXS_Cuts/workspaces', # location of 'allData.root' file
  'cats':'WHLeptonicTag_0,WHLeptonicTag_1,WHLeptonicTag_2', # auto: automatically inferred from input ws
  'catOffset':0, # add offset to category numbers (useful for categories from different allData.root files)  
  'ext':'2022_08_23_STXS', # extension to add to output directory
  'year':'2018', # Use combined when merging all years in category (for plots)

  # Job submission options
  'batch':'condor', # [condor,SGE,IC,local]
  'queue':'espresso' # for condor e.g. microcentury
  
}