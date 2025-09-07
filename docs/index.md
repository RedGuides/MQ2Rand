---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2rand.2022/"
support_link: "https://www.redguides.com/community/threads/mq2rand.76500/"
repository: "https://github.com/RedGuides/MQ2Rand"
config: "MQ2Rand.ini"
authors: "dewey2461, Sic"
tagline: "a \"/random\" helper for raid leaders to determine who won what."
---

# MQ2Rand

<!--desc-start-->
MQ2Rand provides a window that sorts "/random" messages so the raid looter can quickly determine winners. Players in the raid do their /random as normal, and MQ2Rand populates a list. When you click their name it spits out the winner over `/rsay`. It also yells at players who roll the wrong die.
<!--desc-end-->

## Commands

<a href="cmd-random/">
{% 
  include-markdown "projects/mq2rand/cmd-random.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2rand/cmd-random.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2rand/cmd-random.md') }}

## Settings

Settings are kept in MQ2Rand.ini

```ini
[vox.Voxvox]
ChatTop=10
ChatBottom=22
ChatLeft=10
ChatRight=210
Locked=0
Fades=1
Delay=2000
Duration=500
Alpha=255
FadeToAlpha=255
BGType=1
BGTint.alpha=255
BGTint.red=0
BGTint.green=0
BGTint.blue=0
ON=1
```
