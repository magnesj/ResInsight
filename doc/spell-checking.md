# Spell Checking Workflows

ResInsight uses automated spell checking to maintain code quality and catch typos in code, comments, and documentation.

## Two Workflows

### 1. spell-check.yml (Full Repository Check)

**When it runs:** On every push to any branch

**What it does:**
- Checks all code in four directories:
  - `ApplicationExeCode/`
  - `ApplicationLibCode/`
  - `Fwk/AppFwk/`
  - `GrpcInterface/`
- Uses `misspell-fixer` to find and suggest fixes for typos
- Automatically creates a PR with fixes if typos are found
- PR will be created on a branch named `spell-check-patches-*`

**Use case:** Regular maintenance and keeping the entire codebase clean

### 2. spell-check-pr.yml (PR Review Check)

**When it runs:** On pull requests that modify files in the target directories

**What it does:**
- Only checks files that were changed in the PR
- Uses `codespell` for spell checking
- Checks these file types: `.cpp`, `.h`, `.inl`, `.py`, `.md`, `.txt`, `.cmake`, `.yml`, `.yaml`
- Skips binary files: `.svg`, `.xml`, `.json`
- Fails the PR check if typos are found (must be fixed before merge)

**Use case:** Preventing new typos from being introduced in PRs

## Whitelists for Domain-Specific Terms

### .codespell-ignore

Used by `codespell` (PR workflow). Contains domain-specific terms that should not be flagged:

- Petroleum engineering terms: `perm`, `porosity`, `wellbore`, `facies`
- Eclipse file formats: `EGRID`, `INIT`, `UNRST`, `SMSPEC`
- Simulation tools: `Abaqus`, `ODB`, `VTK`
- Common abbreviations: `API`, `GUI`, `RGB`, `CSV`
- Units: `ft`, `mD`, `bbl`, `psi`

**To add a word:** Simply add it to `.codespell-ignore` (one word per line, comments start with `#`)

### .misspell-fixer.ignore

Used by `misspell-fixer` (full repository workflow). Contains file patterns and specific line exceptions:

- File patterns to skip (e.g., `^ApplicationExeCode/Resources/.*\.svg$`)
- Specific file:line:word exceptions
- Third-party directories

**To add an exception:** Add a regex pattern matching the file path

## How to Fix Typos

### From Full Repository Check

When the spell-check workflow creates a PR:

1. Review the PR created by the workflow
2. Check if the suggested changes are correct
3. If correct: merge the PR
4. If incorrect: close the PR and add the word to `.codespell-ignore` or `.misspell-fixer.ignore`

### From PR Review Check

When your PR fails the spell check:

1. Read the error message to see which words are flagged
2. Fix the typos in your code OR
3. If the word is correct (domain-specific term), add it to `.codespell-ignore`
4. Push the changes to your PR
5. The check will run again automatically

## Common Domain Terms Already Whitelisted

The following are already in the whitelist:
- Reservoir simulation: `perm`, `permeability`, `porosity`, `geomech`, `seismic`
- Eclipse formats: `EGRID`, `GRID`, `INIT`, `UNRST`, `SMSPEC`, `ESMRY`
- Tools: `Abaqus`, `ODB`, `VTK`, `OSDU`, `FMU`, `VFP`
- Abbreviations: `Md`, `Tvd`, `Rkb`, `Rft`, `Plt`
- Organizations: `ResInsight`, `Equinor`, `Ceetron`, `OPM`

## Testing Locally

You can run spell checks locally before pushing:

### Using codespell (recommended for quick checks)

```bash
# Install codespell
pip install codespell

# Check specific files
codespell --ignore-words=.codespell-ignore myfile.cpp

# Check a directory
codespell --ignore-words=.codespell-ignore ApplicationLibCode/
```

### Using misspell-fixer (matches the full workflow)

```bash
# Install misspell-fixer
git clone https://github.com/vlajos/misspell_fixer
cd misspell_fixer
export PATH=$PATH:$(pwd)

# Run on a directory
misspell_fixer -rsvnuR /path/to/ResInsight/ApplicationLibCode/
```

## Troubleshooting

### False Positives

If a word is flagged but is correct:
1. Add it to `.codespell-ignore` (for PR checks)
2. Add a pattern to `.misspell-fixer.ignore` (for full checks)

### Words That Should Be Fixed

If the spell checker suggests a change that seems wrong, check:
- Is the word used correctly in context?
- Is it a technical term that should be in the whitelist?
- Is there a better/clearer word to use?

### Workflow Not Running

- Check that your changes touch files in the monitored directories
- Check that the file extensions are supported
- Look at the GitHub Actions tab for error messages
