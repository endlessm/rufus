set stagedir=%1
set builddir=%2
set buildflavor=%3
set nrproc=%4

set commandtorun=b2.exe  -j%nrproc% --stagedir=%stagedir% --build-dir=%builddir% architecture=x86  address-model=32 --toolset=msvc --build-type=minimal variant=%buildflavor% link=static threading=multi runtime-link=static stage

%commandtorun% > nul
%commandtorun%