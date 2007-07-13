#!/usr/bin/perl -w
# $Id$

use strict;

my ($ScriptDir, $ScriptName);

BEGIN
{
    ($ScriptDir, $ScriptName) = $0 =~ m/^(?:(.+)[\\\/])?(.+)$/;
    $ScriptDir ||= '.'
}

use lib $ScriptDir;

use NCBI::SVN::Wrapper;
use NCBI::SVN::Update;
use NCBI::SVN::SwitchMap;
use NCBI::SVN::MultiSwitch;

use File::Spec;
use File::Basename;
use File::Find;
use Cwd;
use Fcntl qw(F_SETFD);

my $DefaultRepos = 'https://svn.ncbi.nlm.nih.gov/repos/toolkit';

my $Update = NCBI::SVN::Update->new(MyName => $ScriptName);

# Find (and cache) the path to Subversion before unsetting
# the PATH environment variable.
$Update->GetSvnPath();

my @UnsafeVars = qw(PATH IFS CDPATH ENV BASH_ENV TERM);
my %OldEnv;
@OldEnv{@UnsafeVars} = delete @ENV{@UnsafeVars};

$ScriptDir = Cwd::realpath($ScriptDir);

my @ProjectDirs = ("$ScriptDir/projects", "$ScriptDir/internal/projects");

sub Usage
{
    my ($Error) = @_;

    print STDERR $ScriptName . ' $Revision$' . <<EOF;

This script checks out files required for building the specified project
and optionally (re-)configures and builds it.

Usage:
    1. $ScriptName [-switch-map SwitchMapFile] Project BuildDir

    2. $ScriptName [-switch-map SwitchMapFile] Project

Where:
    Project - The name of the project you want to build or a pathname
        of the project listing file.

        If this argument looks like a project name (that is, it
        doesn't contain path information), then the '.lst' extension
        is added to it and the resulting file name is searched for
        in the following system directories:

EOF

    my $Indent = ' ' x 8;

    print STDERR "$Indent$_\n" for @ProjectDirs;

    print STDERR "\n${Indent}Available project names are:\n\n";

    my @ProjectNames;
    find(sub {s/\.lst$// && push @ProjectNames, $_}, @ProjectDirs);

    my $MaxWidth = 80 - length($Indent) - 6;

    my $Column = 0;

    for (@ProjectNames)
    {
        unless ($Column > 0)
        {
            while (length($_) > $MaxWidth)
            {
                m/^(.{$MaxWidth})(.+)$/;
                print STDERR "$Indent$1\n";
                $_ = $2
            }

            print STDERR $Indent
        }
        elsif (++$Column + length($_) <= $MaxWidth)
        {
            print STDERR ' '
        }
        else
        {
            if (length($_) > $MaxWidth)
            {
                if (($Column = $MaxWidth - $Column) > 0)
                {
                    m/^(.{$Column})(.+)$/;
                    print STDERR " $1";
                    $_ = $2
                }

                while (length($_) > $MaxWidth)
                {
                    m/^(.{$MaxWidth})(.+)$/;
                    print STDERR "\n$Indent$1";
                    $_ = $2
                }
            }

            print STDERR "\n$Indent";
            $Column = 0
        }

        print STDERR $_;

        if (($Column += length($_)) == $MaxWidth)
        {
            print STDERR "\n";
            $Column = 0
        }
    }

    print STDERR "\n" if $Column > 0;

    print STDERR <<EOF;

    SwitchMapFile - Pathname of the file containing working copy directory
        switch plan.

    BuildDir - Path to the target directory. The presence (or absence)
        of this argument designates the mode of operation - see
        the Description section below.

Description:
    This script supports two modes of operation:

    1. If you specify the BuildDir argument, the script will create the
        directory if necessary and check the specified portion of
        the C++ Toolkit tree out into it, along with any additional
        infrastructure needed for the build system to work.
        If the directory already exists, it must be empty.
        If the SwitchMapFile file is specified, the script will switch
        working copy directories in accordance with this file.
        The script will then optionally configure and build
        the new tree.

    2. If you run this script from the top level of an existing working
        copy of the C++ source tree without sepcifying the BuildDir
        argument, it will update the sources and headers for the
        specified projects.
        If the SwitchMapFile file is specified, the script will switch
        working copy directories in accordance with this file.
        The script will then optionally reconfigure and rebuild the tree.

Examples:
    1. Perform initial checkout of project "connect" and its
        dependencies, then optionally configure and build them:

        \$ $ScriptName connect ./connect_build

    2. Update sources of the "connect" project and all of its
        dependencies to the latest versions from the repository
        and then optionally reconfigure and build them:

        \$ cd connect_build
        \$ $ScriptName connect

EOF

    if ($Error)
    {
        print STDERR "Error:  $Error.\n";
        exit 1
    }

    exit 0
}

my @Paths;

my %AllProjects;

my $RepositoryURL;

my @ProjectQueue;

# Load project listing files, which were either found in the project search
# paths by names of projects they described or were explicitly specified in
# the previously read project listing files using the #include directive.
sub ReadProjectListingFile
{
    my ($FileName, $Context) = @_;

    open FILE, '<', $FileName or die "$Context:$FileName: $!\n";

    while (<FILE>)
    {
        s/\s*$//so;
        s/\$$/\//so;

        $Context = "$FileName:$.";

        if (m/^\s*\&\s*(.*?)/)
        {
            push @ProjectQueue, $1, $Context
        }
        elsif (m/^\s*#\s*include\s+"(.*?)"/)
        {
            push @ProjectQueue, File::Spec->rel2abs($1,
                dirname($FileName)), $Context
        }
        elsif (m/!repo?s?\s+(.*)/)
        {
            $RepositoryURL = $1
        }
        elsif (m/^\.\/(.*)/)
        {
            push @Paths, $1
        }
        else
        {
            $AllProjects{$_} = 1;

            if (m{(?:corelib|dbapi/driver|objects/objmgr|objmgr|serial)/$}o)
            {
                push @Paths, 'include/' . $_ . 'impl'
            }

            push @Paths, 'include/' . $_, 'src/' . $_
        }
    }

    close FILE
}

sub FindProjectListing
{
    my ($Project, $Context) = @_;

    my ($Volume, $Dir, undef) = File::Spec->splitpath($Project);

    return $Project if (($Volume || $Dir) && -f $Project);

    # Search through the registered directories with
    # project listings.
    for my $ProjectDir (@ProjectDirs)
    {
        my $FileName = "$ProjectDir/$Project.lst";

        return $FileName if -f $FileName
    }

    die "$Context: unable to find project '$Project'\n"
}

Usage() if @ARGV < 1 || $ARGV[0] eq '--help';

my ($SwitchMapFile, $MainProject, $BuildDir);

while (@ARGV)
{
    my $Arg = shift @ARGV;

    if ($Arg eq '-switch-map' || $Arg eq '-branches')
    {
        $SwitchMapFile = shift @ARGV or Usage("Pathname missing after $Arg")
    }
    elsif (!$MainProject)
    {
        $MainProject = $Arg
    }
    elsif (!$BuildDir)
    {
        $BuildDir = $Arg
    }
    else
    {
        Usage('Too many command line arguments')
    }
}

Usage('Missing mandatory argument Project') unless $MainProject;

$MainProject = FindProjectListing($MainProject, $ScriptName);

ReadProjectListingFile($MainProject, $ScriptName);

my $NewCheckout;

if ($BuildDir)
{
    die "Error: $BuildDir is not empty.\n" if -d "$BuildDir/.svn";

    mkdir $BuildDir;

    my $ProjectListFile = "$BuildDir/projects";
    if (eval {symlink('', ''); 1})
    {
        unlink $ProjectListFile;
        symlink(File::Spec->file_name_is_absolute($MainProject) ?
            $MainProject : File::Spec->rel2abs($MainProject), $ProjectListFile)
            or die $!;
    }
    else
    {
        open IN, '<', $MainProject or die "$MainProject: $!\n";
        open OUT, '>', $ProjectListFile or die "$ProjectListFile: $!\n";
        print OUT while <IN>;
        close OUT;
        close IN
    }

    push @Paths, qw(./compilers ./scripts ./include/common);

    $RepositoryURL = $DefaultRepos;
    $NewCheckout = 1
}
else
{
    die "No directory specified and no existing checkout detected.\n"
        unless -d '.svn' && -e 'projects';

    $BuildDir = '.'
}

# For each project included by an ampersand reference.
while (@ProjectQueue)
{
    my $Project = shift @ProjectQueue;
    my $Context = shift @ProjectQueue;

    ReadProjectListingFile(FindProjectListing($Project, $Context), $Context)
}

my $HEAD = NCBI::SVN::Wrapper->new()->GetLatestRevision();

if ($RepositoryURL)
{
    $Update->RunSubversion(($NewCheckout ? 'co' : 'switch'), '-r', $HEAD,
        '-N', "$RepositoryURL/trunk/c++", $BuildDir)
}

my $SwitchMap;

$SwitchMap = NCBI::SVN::SwitchMap->new(MyName => $ScriptName,
    MapFileName => $SwitchMapFile) if $SwitchMapFile;

chdir $BuildDir;

$Update->UpdateDirList($HEAD, @Paths);

NCBI::SVN::MultiSwitch->new(MyName => $ScriptName)->
    SwitchUsingMap($SwitchMap->GetSwitchPlan()) if $SwitchMap;

exit 0 if $^O eq 'MSWin32';

while (my ($Var, $Val) = each %OldEnv)
{
    $ENV{$Var} = $Val if defined $Val
}

sub ConfirmYes
{
    my ($Prompt) = @_;

    print "\n$Prompt (y/n)\n";

    my $Answer;

    do
    {
        $Answer = readline STDIN;

        exit 0 if $Answer =~ m/^n/i
    }
    until ($Answer =~ m/^y/i)
}

sub RunOrDie
{
    my ($CommandLine) = @_;

    if (system($CommandLine) != 0)
    {
        die "$ScriptName\: " .
            ($? == -1 ? "failed to execute $CommandLine\: $!" :
            $? & 127 ? "'$CommandLine' died with signal " . ($? & 127) :
            "'$CommandLine' exited with status " . ($? >> 8)) . "\n"
    }
}

sub Menu
{
    my ($Prompt, $Base, @Items) = @_;

    print "\n$Prompt\n";

    my $MaxChoice = $Base;

    for my $Item (@Items)
    {
        print $MaxChoice++ . " - $Item\n";
    }

    --$MaxChoice;

    for (;;)
    {
        my $Choice = readline STDIN;

        return $Choice if $Choice >= $Base && $Choice <= $MaxChoice;

        print "Please give a number between 1 and $MaxChoice.\n";
    }
}

my @ExistingBuilds;

sub FindExistingBuilds
{
    opendir DIR, '.' or die "$ScriptName\: can't opendir '.': $!\n";
    @ExistingBuilds = grep {!m/^\./ && -d $_ . '/build'} readdir DIR;
    closedir DIR
}

FindExistingBuilds();

unless (@ExistingBuilds)
{
    # No builds; presumably a new checkout
    ConfirmYes('Would you like to configure this tree?');

    my $Platform = `uname -sm`;
    chomp $Platform;

    my @AvailableConfigs =
        $Platform =~ m/^SunOS / ?
            ('./configure', 'compilers/WorkShop.sh 32',
                'compilers/WorkShop.sh 64') :
        $Platform =~ m/^Linux i.86/ ?
            ('./configure', 'compilers/ICC.sh', 'compilers/KCC.sh 32') :
        $Platform =~ m/^IRIX / ?
            ('./configure', 'compilers/MIPSpro73.sh 32',
                'compilers/MIPSpro73.sh 64') :
        $Platform =~ m/ alpha$/ ?
            ('./configure', 'compilers/Compaq.sh') : ('./configure');

    my $Configure = $AvailableConfigs[@AvailableConfigs == 1 ? 0 :
        Menu('Which configure script would you like to use?',
            1 => @AvailableConfigs) - 1];

    if ($Configure =~ m/\/ICC/)
    {
        my $IA32ROOT = '/opt/intel/compiler80';

        @ENV{qw(IA32ROOT PATH LD_LIBRARY_PATH INTEL_FLEXLM_LICENSE)} =
            ($IA32ROOT, "$IA32ROOT/bin:$ENV{PATH}",
                "$IA32ROOT/lib:$ENV{LD_LIBRARY_PATH}",
                '/opt/intel/licenses')
    }
    elsif ($Configure =~ m/\/KCC/)
    {
        $ENV{PATH} = '/usr/kcc/KCC_BASE/bin:' . $ENV{PATH}
    }

    my @ConfigOptions;

    for my $OptProject (qw(dbapi serial objects app internal))
    {
        push @ConfigOptions, ($AllProjects{$OptProject} ?
            '--with-' : '--without-') . $OptProject
    }

    my $ConfigOptions = join(' ', @ConfigOptions);

    print "Current options to configure (deduced from project list):\n" .
        "$ConfigOptions\nIf you would like to pass any additional options, " .
        "please enter them now;\notherwise, just hit return.\n";

    my $MoreOptions = readline STDIN;
    chomp $MoreOptions;

    $ConfigOptions .= ' ' . $MoreOptions if $MoreOptions;

    RunOrDie("$Configure $ConfigOptions");

    FindExistingBuilds();

    die "$ScriptName\: configure did not result in a single build directory\n"
        if @ExistingBuilds != 1;

    ConfirmYes('Would you like to compile this tree?');

    chdir $ExistingBuilds[0] . '/build' or die
}
else
{
    my $Choice = Menu('Would you like to reconfigure an existing ' .
        'build of this tree?', 0 => 'No, thank you.',
        map {"Yes, please reconfigure $_."} @ExistingBuilds);

    exit 0 if $Choice == 0;

    chdir $ExistingBuilds[$Choice - 1] . '/build';

    chmod 0755, 'reconfigure.sh';

    RunOrDie('./reconfigure.sh reconf');

    ConfirmYes('Would you like to recompile this build?')
}

RunOrDie('make all_p');

ConfirmYes('Would you like to run tests on this build?');

RunOrDie('make check_p');

exit 0
