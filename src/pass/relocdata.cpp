#include "relocdata.h"
#include "elf/elfmap.h"
#include "elf/reloc.h"
#include "chunk/aliasmap.h"
#include "conductor/conductor.h"
#include "log/log.h"

Function *FindAnywhere::findAnywhere(const char *target) {
    if(!conductor) return nullptr;

    for(auto library : *conductor->getLibraryList()) {
        auto elfSpace = library->getElfSpace();
        if(!elfSpace) continue;
        auto module = elfSpace->getModule();

        auto found = module->getChildren()->getNamed()->find(target);
        if(!found) {
            found = elfSpace->getAliasMap()->find(target);
        }

        if(found) {
            this->elfSpace = elfSpace;
            return found;
        }
    }

    LOG(1, "    could not find " << target << " ANYWHERE");
    return nullptr;
}

void RelocDataPass::visit(Module *module) {
    for(auto r : *relocList) {
        if(!r->getSymbol() || !*r->getSymbol()->getName()) continue;

        LOG(1, "trying to fix " << r->getSymbol()->getName());

        FindAnywhere found(conductor);
        auto target = found.findAnywhere(r->getSymbol()->getName());
        if(!target) continue;

        LOG(1, "FOUND ANYWHERE " << r->getSymbol()->getName());

        fixRelocation(r, target, found);
    }
}

void RelocDataPass::fixRelocation(Reloc *r, Function *target,
    FindAnywhere &found) {

#ifdef ARCH_X86_64
    if(r->getType() == R_X86_64_GLOB_DAT) {
        address_t update = elf->getBaseAddress() + r->getAddress();
        address_t dest = found.getElfSpace()->getElfMap()->getBaseAddress()
            + target->getAddress();
        LOG(1, "fix address " << update << " to point at "
            << dest);
        *(unsigned long *)update = dest;
    }
    else if(r->getType() == R_X86_64_JUMP_SLOT) {
        address_t update = elf->getBaseAddress() + r->getAddress();
        address_t dest = target->getAddress();
        LOG(1, "fix address " << update << " to point at "
            << dest);
        *(unsigned long *)update = dest;
    }
    else {
        LOG(1, "NOT fixing because type is " << r->getType());
    }
#endif
}
