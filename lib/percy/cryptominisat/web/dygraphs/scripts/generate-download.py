#!/usr/bin/env python

# Generates docs/download.html
# Run:
# ./generate-download.py > docs/download.html

import json

releases = json.load(file('releases.json'))

def file_links(release):
  v = release['version']
  return ['<a href="%(v)s/%(f)s">%(f)s</a>' % {
    'f': f, 'v': v} for f in release['files']]


# Validation of releases.json
for idx, release in enumerate(releases):
  if idx == 0: continue
  assert 'version' in release, 'Release missing version: %s' % release
  assert 'files' in release, 'Release missing files: %s' % release
  assert release['version'] < releases[idx - 1]['version'], (
      'Releases should be in reverse chronological order in releases.json')

current_html = '<p>' + ('</p><p>'.join(file_links(releases[0]))) + '</p>'


previous_lis = []
for release in releases[1:]:
  previous_lis.append('<li>%(v)s: %(files)s (<a href="%(v)s/">%(v)s docs</a>)' % {
      'v': release['version'],
      'files': ', '.join(file_links(release))
    })


print '''
<!--#include virtual="header.html" -->

<!--
  DO NOT EDIT THIS FILE!

  This file is generated by generate-download.py.
-->

<script src="modernizr.custom.18445.js"></script>
<p>The current version of dygraphs is <b>%(version)s</b>. Most users will want to download minified files for this version:</p>

<div id="current-release" class="panel">
%(current_html)s
</div>

<p>There's a hosted version of dygraphs on <a href="https://cdnjs.com/libraries/dygraph">cdnjs.com</a>:</p>

<pre>&lt;script src="//cdnjs.cloudflare.com/ajax/libs/dygraph/%(version)s/dygraph.min.js"&gt;&lt;/script&gt;
&lt;link rel=&quot;stylesheet&quot; src=&quot;//cdnjs.cloudflare.com/ajax/libs/dygraph/%(version)s/dygraph.min.css&quot; /&gt;
</pre>

<p>You can also install dygraphs using <a href="https://www.npmjs.org/package/dygraphs">NPM</a>:</p>

<pre>$ npm install dygraphs
# dygraphs is now in node_modules/dygraphs/dygraph.js</pre>

<p>Most distributions include a source map to facilitate debugging.</p>

<p>To generate your own minified JS, run:</p>

<pre>git clone https://github.com/danvk/dygraphs.git
npm run build
</pre>

<p>This will create a dygraph.min.js file in the <code>dist</code> directory.</p>

<p>You may also download files for previously-released versions:</p>

<ul>
%(previous_lis)s
</ul>

<p>See <a href="/versions.html">Version History</a> for more information on each release.</p>


<!--#include virtual="footer.html" -->
''' % {
    'version': releases[0]['version'],
    'current_html': current_html,
    'previous_lis': '\n'.join(previous_lis)
    }
