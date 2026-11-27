<#
Exercises the real PowerShell builder with isolated native compiler/linker
stubs. Requires PowerShell and a POSIX C compiler (cc); no Python dependency.
This validates orchestration/response files, not MSVC or Windows PowerShell5.1.
#>
[CmdletBinding()]
param([string]$CCompiler = 'cc')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$TestRepository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$TestBuilder = Join-Path $TestRepository 'tools/build_windows.ps1'
$TestTemporary = Join-Path ([IO.Path]::GetTempPath()) ('codport PS test Unicode-' + [char]0x044F + ' ' + [Guid]::NewGuid().ToString('N'))
$TestOldPath = $env:PATH
$TestOldTrace = $env:CODPORT_FAKE_TRACE
$TestOldFail = $env:CODPORT_FAKE_FAIL
function Assert-BuildTest([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Write-TestText([string]$Path, [string]$Text) {
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path)) | Out-Null
    [IO.File]::WriteAllText($Path, $Text, (New-Object Text.UTF8Encoding($false)))
}
function Assert-BuildThrows([scriptblock]$Action, [string]$Pattern) {
    $Caught = $null
    try { & $Action } catch { $Caught = $_.Exception.Message }
    Assert-BuildTest ($null -ne $Caught -and $Caught -match $Pattern) ('Expected failure /' + $Pattern + '/, got: ' + $Caught)
}
$TestFakeSource = @'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
static void die(const char *s){fprintf(stderr,"fake tool: %s\n",s);exit(91);}
static int ends(const char*s,const char*t){size_t a=strlen(s),b=strlen(t);return a>=b&&!strcmp(s+a-b,t);}
static void exists(const char*p){FILE*f=fopen(p,"rb");if(!f){fprintf(stderr,"missing input: %s\n",p);exit(92);}fclose(f);}
static void output(const char*p){FILE*f=fopen(p,"wb");if(!f)die("cannot create output");fputs("fake artifact\n",f);fclose(f);}
static char *response(const char*p){FILE*f=fopen(p,"rb");long n;unsigned char*b;char*s;size_t i,k=0;
 if(!f)die("cannot open response");fseek(f,0,SEEK_END);n=ftell(f);rewind(f);if(n<2||n%2)die("invalid response length");
 b=malloc((size_t)n);s=malloc((size_t)n*2+1);if(!b||!s)die("allocation");if(fread(b,1,(size_t)n,f)!=(size_t)n)die("read");fclose(f);
 if(b[0]!=255||b[1]!=254)die("response is not UTF16LE BOM");
 for(i=2;i<(size_t)n;i+=2){uint32_t cp=b[i]|((uint32_t)b[i+1]<<8);
  if(cp>=0xd800&&cp<=0xdbff&&i+3<(size_t)n){uint32_t low=b[i+2]|((uint32_t)b[i+3]<<8);if(low<0xdc00||low>0xdfff)die("surrogate");cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);i+=2;}
  if(cp<128)s[k++]=(char)cp;else if(cp<2048){s[k++]=(char)(192|(cp>>6));s[k++]=(char)(128|(cp&63));}
  else if(cp<65536){s[k++]=(char)(224|(cp>>12));s[k++]=(char)(128|((cp>>6)&63));s[k++]=(char)(128|(cp&63));}
  else{s[k++]=(char)(240|(cp>>18));s[k++]=(char)(128|((cp>>12)&63));s[k++]=(char)(128|((cp>>6)&63));s[k++]=(char)(128|(cp&63));}}
 s[k]=0;free(b);return s;}
static int words(char*s,char***out){char**v=calloc(strlen(s)+1,sizeof(*v));int n=0;char*p=s,*d=s;
 if(!v)die("words allocation");while(*p){int quoted=0;while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n')p++;if(!*p)break;v[n++]=d;
  while(*p&&(quoted||(*p!=' '&&*p!='\t'&&*p!='\r'&&*p!='\n'))){unsigned slash=0;while(*p=='\\'){slash++;p++;}
   if(*p=='"'){while(slash>=2){*d++='\\';slash-=2;}if(slash)*d++='"';else quoted=!quoted;p++;}
   else{while(slash--)*d++='\\';if(*p&&(quoted||(*p!=' '&&*p!='\t'&&*p!='\r'&&*p!='\n')))*d++=*p++;}}
  if(quoted)die("unbalanced response quote");if(*p)p++;*d++=0;}
 *out=v;return n;}
static void compile_source(const char*p){char object[8192],header[8192],line[256],actual[256];const char*base=strrchr(p,'/');char*extension;FILE*f;
 exists(p);base=base?base+1:p;snprintf(object,sizeof(object),"%s",base);extension=strrchr(object,'.');if(!extension)die("source name");strcpy(extension,".obj");output(object);
 f=fopen(p,"rb");if(!f)die("source read");line[0]=0;fgets(line,sizeof(line),f);fclose(f);
 if(!strncmp(line,"EXPECT_HEADER=",14)){char*slash;snprintf(header,sizeof(header),"%s",p);slash=strrchr(header,'/');if(!slash)die("source path");strcpy(slash+1,"shared.h");
  f=fopen(header,"rb");if(!f)die("header missing");actual[0]=0;fgets(actual,sizeof(actual),f);fclose(f);
  line[strcspn(line,"\r\n")]=0;actual[strcspn(actual,"\r\n")]=0;if(strcmp(line+14,actual))die("historical header not restored");}}
int main(int argc,char**argv){char*s,**v;const char*tool=strrchr(argv[0],'/'),*trace=getenv("CODPORT_FAKE_TRACE"),*fail=getenv("CODPORT_FAKE_FAIL");FILE*f;int i,n;
 tool=tool?tool+1:argv[0];if(argc!=2||argv[1][0]!='@')die("expected ONE @response argument");s=response(argv[1]+1);n=words(s,&v);
 f=trace?fopen(trace,"ab"):NULL;if(f){fprintf(f,"TOOL|%s\n",tool);for(i=0;i<n;i++)fprintf(f,"ARG|%s\n",v[i]);fclose(f);}
 puts("fake native stdout");fputs("fake native stderr\n",stderr);if(fail&&!strcmp(fail,tool)){free(v);free(s);return 37;}
 for(i=0;i<n;i++){if(!strcmp(tool,"cl.exe")&&(ends(v[i],".c")||ends(v[i],".cpp")||ends(v[i],".cc")))compile_source(v[i]);
  if(strcmp(tool,"cl.exe")&&ends(v[i],".obj"))exists(v[i]);
  if(!strncmp(v[i],"/OUT:",5))output(v[i]+5);}
 free(v);free(s);return 0;}
'@
try {
    [IO.Directory]::CreateDirectory($TestTemporary) | Out-Null
    $TestTools = Join-Path $TestTemporary 'fake native tools'
    [IO.Directory]::CreateDirectory($TestTools) | Out-Null
    $TestFakeC = Join-Path $TestTemporary 'fake tool.c'
    Write-TestText $TestFakeC $TestFakeSource
    $TestCompiler = (Get-Command $CCompiler -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    $TestCl = Join-Path $TestTools 'cl.exe'
    & $TestCompiler '-std=c11' '-O2' $TestFakeC '-o' $TestCl
    Assert-BuildTest ($LASTEXITCODE -eq 0) 'Cannot compile native test stub.'
    Copy-Item -LiteralPath $TestCl -Destination (Join-Path $TestTools 'lib.exe')
    Copy-Item -LiteralPath $TestCl -Destination (Join-Path $TestTools 'link.exe')
    $env:PATH = $TestTools + [IO.Path]::PathSeparator + $TestOldPath
    $env:CODPORT_FAKE_TRACE = Join-Path $TestTemporary 'native args.txt'
    $env:CODPORT_FAKE_FAIL = $null
    . $TestBuilder

    $TestFixture = Join-Path $TestTemporary 'fixture source'
    Write-TestText (Join-Path $TestFixture 'src/shared.h') "CURRENT`n"
    Write-TestText (Join-Path $TestFixture 'src/a.c') "EXPECT_HEADER=OLD`n"
    Write-TestText (Join-Path $TestFixture 'src/b.c') "EXPECT_HEADER=CURRENT`n"
    Write-TestText (Join-Path $TestFixture 'src/game.c') "EXPECT_HEADER=CURRENT`n"
    Write-TestText (Join-Path $TestFixture 'revisions/old/shared.h') "OLD`n"
    $TestSpecialArguments = @('/DSPACE=hello world', '/DQUOTED=a"b', '/DTRAIL=C:\a b\')
    $TestRecipe = [ordered]@{
        support = [ordered]@{
            output = 'support.lib'
            compile = @(
                @{flags = @('/nologo', '/c') + $TestSpecialArguments; sources = @('src/a.c'); headers = 'old'},
                @{flags = @('/c'); sources = @('src/b.c')},
                @{flags = @('/c'); sources = @('src/a.c'); headers = 'old'}
            )
            objects = @('src/a.c', 'src/b.c')
            link = @('/nologo', '@objects')
        }
        game = [ordered]@{
            output = 'gamex86.dll'
            compile = @(@{flags = @('/c'); sources = @('src/game.c')})
            objects = @('src/game.c')
            link = @('/DLL', '@objects', '@support')
        }
    }
    Write-TestText (Join-Path $TestFixture 'tools/windows-x86.json') ($TestRecipe | ConvertTo-Json -Depth 20)
    Write-TestText (Join-Path $TestFixture 'tools/windows-headers.json') (@{old = @{'src/shared.h' = 'revisions/old/shared.h'}} | ConvertTo-Json -Depth 10)
    Write-TestText (Join-Path $TestFixture 'main/ui/menudef.h') 'test menu header'
    Write-TestText (Join-Path $TestFixture 'main/training_player.cfg') 'test training config'
    foreach ($TestName in @('wolfconfig.cfg', 'Play-Training.cmd', 'Play-Training-Single.cmd', 'Play-Training-Multi.cmd', 'Play-Training-Native.cmd', 'Play-Training.ps1', 'THREADING-MODES.md')) {
        Write-TestText (Join-Path $TestFixture ('packaging/windows/' + $TestName)) ('fixture ' + $TestName)
    }
    $TestPgd = Join-Path $TestTemporary 'profile input.pgd'
    $TestOpenAL = Join-Path $TestTemporary 'OpenAL input.dll'
    Write-TestText $TestPgd 'profile-data'
    Write-TestText $TestOpenAL 'openal-data'
    $TestOutput = Join-Path $TestTemporary 'fixture output'
    Invoke-WindowsBuild -Root $TestFixture -Build $TestOutput -GamePgd $TestPgd -OpenAL $TestOpenAL
    $TestArguments = [IO.File]::ReadAllLines($env:CODPORT_FAKE_TRACE)
    foreach ($TestArgument in $TestSpecialArguments) {
        Assert-BuildTest ($TestArguments -contains ('ARG|' + $TestArgument)) ('Response-file quote roundtrip failed: ' + $TestArgument)
    }
    Assert-BuildTest ([IO.File]::ReadAllText((Join-Path $TestFixture 'src/shared.h')) -eq "CURRENT`n") 'Builder modified source header.'
    Assert-BuildTest ([IO.File]::ReadAllText((Join-Path $TestOutput 'obj/inputs/src/shared.h')) -eq "CURRENT`n") 'Builder did not restore current header.'
    Assert-BuildTest ([IO.File]::ReadAllText((Join-Path $TestOutput 'obj/game/training.pgd')) -eq 'profile-data') 'PGD copy differs.'
    Assert-BuildTest ([IO.File]::ReadAllText((Join-Path $TestOutput 'bin/soft_oal.dll')) -eq 'openal-data') 'OpenAL copy differs.'
    foreach ($TestRelative in @('obj/support/support.lib','bin/gamex86.dll','profile/main/training_player.cfg','profile/main/wolfconfig.cfg','profile-native/main/wolfconfig.cfg','Play-Training-Single.cmd','Play-Training-Multi.cmd','Play-Training-Native.cmd','Play-Training.ps1')) {
        Assert-BuildTest (Test-Path -LiteralPath (Join-Path $TestOutput $TestRelative)) ('Missing output: ' + $TestRelative)
    }
    Assert-BuildTest (-not (Test-Path -LiteralPath (Join-Path $TestOutput 'profile-native/main/training_player.cfg'))) 'Native profile must not contain Training CFG.'
    Assert-BuildTest ([IO.File]::ReadAllText((Join-Path $TestOutput 'profile-native/main/wolfconfig.cfg')) -eq [IO.File]::ReadAllText((Join-Path $TestOutput 'profile/main/wolfconfig.cfg'))) 'Native seed settings differ.'
    $TestCompileLog = [IO.File]::ReadAllText((Join-Path $TestOutput 'obj/support/compile-0.log'))
    Assert-BuildTest ($TestCompileLog -match 'fake native stdout' -and $TestCompileLog -match 'fake native stderr') 'Native streams are not both in build log.'
    Assert-BuildThrows { Invoke-WindowsBuild -Root $TestFixture -Build $TestOutput -GamePgd '' -OpenAL '' } 'empty|not empty|nonempty|already|exists'
    Write-Host 'PASS synthetic recipe, UTF16LE/Unicode/quotes, header restoration, native logs, profile/assets and occupied output guard'

    $env:CODPORT_FAKE_FAIL = 'cl.exe'
    $TestFailureOutput = Join-Path $TestTemporary 'expected native failure'
    Assert-BuildThrows { Invoke-WindowsBuild -Root $TestFixture -Build $TestFailureOutput -GamePgd '' -OpenAL '' } '37|failed'
    Assert-BuildTest (Test-Path -LiteralPath (Join-Path $TestFailureOutput 'obj/support/compile-0.log')) 'Missing failed compilation log.'
    Assert-BuildTest (-not (Test-Path -LiteralPath (Join-Path $TestFailureOutput 'bin/gamex86.dll'))) 'Build continued to produce game after native failure.'
    $env:CODPORT_FAKE_FAIL = $null
    Write-Host 'PASS native error propagation and early stop'

    $TestRealOutput = Join-Path $TestTemporary 'real recipe output'
    $TestInvocationsBefore = @([IO.File]::ReadAllLines($env:CODPORT_FAKE_TRACE) | Where-Object { $_ -like 'TOOL|*' }).Count
    Invoke-WindowsBuild -Root $TestRepository -Build $TestRealOutput -GamePgd '' -OpenAL ''
    $TestRealRecipe = Get-Content -LiteralPath (Join-Path $TestRepository 'tools/windows-x86.json') -Raw | ConvertFrom-Json
    $TestExpectedInvocations = 0
    foreach ($TestTarget in $TestRealRecipe.PSObject.Properties) {
        $TestExpectedInvocations += @($TestTarget.Value.compile).Count + 1
        $TestRelative = if ($TestTarget.Value.output.EndsWith('.lib')) { 'obj/' + $TestTarget.Name + '/' + $TestTarget.Value.output } else { 'bin/' + $TestTarget.Value.output }
        Assert-BuildTest (Test-Path -LiteralPath (Join-Path $TestRealOutput $TestRelative)) ('Real recipe missing product: ' + $TestRelative)
    }
    $TestInvocationsAfter = @([IO.File]::ReadAllLines($env:CODPORT_FAKE_TRACE) | Where-Object { $_ -like 'TOOL|*' }).Count
    Assert-BuildTest (($TestInvocationsAfter - $TestInvocationsBefore) -eq $TestExpectedInvocations) 'Native invocation count differs from recipe.'
    Write-Host ('PASS complete real JSON recipe: ' + $TestExpectedInvocations + ' stub compiler/linker invocations (no MSVC compilation claimed)')
}
finally {
    $env:PATH = $TestOldPath
    $env:CODPORT_FAKE_TRACE = $TestOldTrace
    $env:CODPORT_FAKE_FAIL = $TestOldFail
    if (Test-Path -LiteralPath $TestTemporary) { Remove-Item -LiteralPath $TestTemporary -Recurse -Force }
}
