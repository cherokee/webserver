SUFFIXES = .py.pre .py

.py.pre.py:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%sbindir%|${sbindir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%cgiroot%|${CGI_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%pythonpath%|${PYTHON}|g; s|%phpcgi%|${PHPCGI}|g; s|%datadir%|${datadir}|g; s|%localedir%|${localedir}|g" $< > $@

PY_PRE = \
conf.py.pre

PYS = \
$(PY_PRE:.py.pre=.py)

DISTCLEANFILES = \
$(PYS)

all-local: $(PYS)

EXTRA_DIST = \
$(PY_PRE) \
base.py \
util.py \
help.py \
pyscgi.py \
fcgi.py \
run-tests.py \
001-basic.py \
002-MissingProtocol.py \
003-UnknownMethod.py \
004-UnknownMethod+MissingProtocol.py \
005-Unknown-Protocol.py \
006-Wrong-Protocol.py \
007-Missing-Host.py \
008-Broken-Request.py \
009-DotDot.py \
010-Dot.py \
011-HeaderEntryTwice.py \
012-Broken-Key.py \
013-Broken-Key2.py \
014-Broken-Key3.py \
015-CGI-simple.py \
016-CGI-pathinfo.py \
017-CGI-pathinfo2.py \
018-CGI-pathinfo3.py \
019-HeadMethod.py \
020-VirtualHost.py \
021-VirtualHost2.py \
022-NoHost.py \
023-Host.py \
024-Host+Port.py \
025-Host-Broken.py \
026-Host-Broken-Dot.py \
027-Missing-File.py \
028-Missing-File+Param.py \
029-Post+Without-length.py \
030-Post+urlencoded.py \
031-Post+urlencoded2.py \
032-DirIndex+Redir.py \
033-DirIndex.py \
034-DirIndex2.py \
035-PHP-simple.py \
036-DocumentRoot-common.py \
037-PHP-post.py \
038-PHP-get.py \
039-Auth-Basic.py \
040-Auth-Basic2.py \
041-Auth-Basic3.py \
042-Pipelining.py \
043-Pipelining2.py \
044-Home.py \
045-Home2.py \
046-Redir.py \
047-Redir2.py \
048-NN.py \
049-NN-broken.py \
050-HeadBody.py \
051-10kRandom.py \
052-50kRandom.py \
053-100kRandom.py \
054-ContentRange.py \
055-ContentRange2.py \
056-ContentRange3.py \
057-ContentRange4.py \
058-ContentRange-Double.py \
059-ContentRange-Double2.py \
060-BigPost.py \
061-PAM.py \
062-PAM2.py \
063-DocumentRoot.py \
064-DirectoryIndex.py \
065-DirectoryIndex2.py \
066-DirectoryIndex3.py \
067-PathInfo.py \
068-PathInfo2.py \
069-PathInfo3.py \
070-Inherit.py \
071-Inherit2.py \
072-Inherit3.py \
073-Access.py \
074-Access2.py \
075-AccessRange.py \
076-AccessRange2.py \
077-AccessRange3.py \
078-AccessAny.py \
079-Extension.py \
080-Extension2.py \
081-DocumentRoot-Common-index.py \
082-Auth-header.py \
083-RedirParam.py \
084-RedirParamShow.py \
085-AuthUsers.py \
086-AuthUsers2.py \
087-Location.py \
088-CGI-envs.py \
089-gzip-encoding.py \
090-Digest.py \
091-Digest2.py \
092-Digest3.py \
093-InvalidLinks.py \
094-URL-request.py \
095-htpasswd-apache-md5.py \
096-htpasswd-md5.py \
097-htpasswd-sha1.py \
098-htpasswd-crypt.py \
099-htpasswd-plain.py \
100-Digest-htdigest.py \
101-HugePost.py \
102-QueryString.py \
103-ScriptName.py \
104-ScriptAlias_PathInfo.py \
105-If-Range.py \
106-If-Range2.py \
107-Priority1.py \
108-Priority2.py \
109-Priority3.py \
110-Priority4.py \
111-Compac.py \
112-NiceError-CGI.py \
113-HeaderFile.py \
114-HeaderFile2.py \
115-Respin.py \
116-RequestTiny.py \
117-DomainToDir.py \
118-Request-redir.py \
119-Dotdot2.py \
120-Dotdot3.py \
121-NoUserDir.py \
122-Methods.py \
123-ImplicitRedit.py \
124-DomainToDir2.py \
125-domain-redir1.py \
126-SCGI.py \
127-SCGI-Post.py \
128-ValidMethod-common.py \
129-Basic-EmptyPasswd.py \
130-Digest-EmptyPasswd.py \
131-PHP-zero-len-post+Redir.py \
132-file-ContentType.py \
133-If-None-Match.py \
134-HTTP_headers.py \
135-RedirParams.py \
136-BadRange.py \
137-Mime1.py \
138-ChangeUser-cgi.py \
139-domain-wildcards1.py \
140-SCGI-EmptyVars.py \
141-FastCGI-EmptyVars.py \
142-SCGI-ExtraVars.py \
143-ContentRange-NoIO.py \
144-ContentRange2-NoIO.py \
145-ContentRange3-NoIO.py \
146-ContentRange4-NoIO.py \
147-ContentRange-Double-NoIO.py \
148-ContentRange-Double2-NoIO.py \
149-Common-PathInfo.py \
150-Common-No-PathInfo.py \
151-SCGI-vars.py \
152-SCGI-vars2.py \
153-SCGI-vars-root.py \
154-DirRedirection.py \
155-DirRedirection2.py \
156-Mime2.py \
157-Dirlist-symlinks.py \
158-RuleHeader1.py \
159-RuleHeader2.py \
160-RuleNot1.py \
161-RuleNot2.py \
162-RuleAnd1.py \
163-RuleAnd2.py \
164-RuleOr1.py \
165-RuleOr2.py \
166-xsendfile1.py \
167-RuleGeoIP.py \
168-Deflate-Encoding.py \
169-Pipermail.py \
170-Error-ContentLength.py \
171-SCGI-Keepalive.py \
172-Chunked-CGI.py \
173-Chunked-SCGI.py \
174-Respins-Limit.py \
175-Digest-htdigest-2.py \
176-AuthLista1.py \
177-AuthLista2.py \
178-AuthLista3.py \
179-RuleExists.py \
180-RuleExists2.py \
181-RuleExists3.py \
182-SSI-include-file.py \
183-SSI-include-virtual.py \
184-SSI-fsize-file.py \
185-SSI-fsize-virtual.py \
186-SSI-droot.py \
187-Method.py \
188-Method2.py \
189-Redir12.py \
190-RuleExists4.py \
191-RuleExists5.py \
192-RedirDocumentRoot.py \
193-PHP-documentroot.py \
194-PHP-zero-len-post.py \
195-ServerAddr.py \
196-Mime3.py \
197-SOAPAction.py \
198-HTTPresponse.py \
199-ErrorRedir.py \
200-ErrorRedir2.py \
201-PostLimit.py \
202-xsendfile2.py \
203-secdownload1.py \
204-secdownload2.py \
205-secdownload3.py \
206-AuthType.py \
207-RuleExists4.py \
208-Userdir_Pathinfo.py \
209-ScriptAlias_DRoot.py \
210-Userdir_ScriptName.py \
211-EmptyGif.py \
212-DirList-Hidden.py \
213-Post-Chunked1.py \
214-Post-Chunked2.py \
215-Post-Chunked3.py \
216-NameSpace.py \
217-DoubleEncode.py \
218-DrupalRedirect.py \
219-Ext-Dot-Pathinfo.py \
220-Pathinfo-Slash.py \
221-ReHost1.py \
222-EVHost1.py \
223-EVHost2.py \
224-EVHost3.py \
225-PathTranslated.py \
226-PathInfo-Default.py \
227-Redir-Paths.py \
228-Redir-Paths2.py \
229-Fullpath.py \
230-Fullpath2.py \
231-POST-4extra.py \
232-X-Real-IP-CGI.py \
233-X-Real-IP-CGI-2.py \
234-X-Real-IP-CGI-3.py \
235-URL-arg_match1.py \
236-URL-arg_match2.py \
237-URL-arg_match_all1.py \
238-URL-arg_match_all2.py \
239-utf8-file1.py \
240-utf8-dir1.py \
241-utf8-pathinfo1.py \
242-utf8-pathinfo2.py \
243-custom-error-1.py \
244-Directory-Overwrite.py \
245-0x9b.py \
246-Dup-CacheControl.py \
247-UTF8-QueryString.py \
248-Colon-UTF8.py \
249-https-redir.py \
250-https-redir-2.py \
251-php-localcheck.py \
252-XForwardedFor-1.py \
253-XForwardedFor-2.py \
254-XForwardedFor-3.py \
255-header_op-add1.py \
256-header_op-add2.py \
257-header_op-add3.py \
258-header_op-add4.py \
259-Vrule-OR-1.py \
260-Vrule-OR-2.py \
261-ErrorRedir-Default1.py \
262-Redir-order-1.py \
263-If_Modified_Since-Invalid1.py \
264-Nocache-redirs.py \
265-Unset-encoders.py \
266-Forbid-encoders.py \
267-PortRedirections-1.py \
268-Options-PHP1.py \
269-Options-Dirlist1.py \
270-Options-asterisk1.py \
271-full-header-check1.py \
272-Nick-match.py \
273-Unknown-method-redir.py \
274-header_op-remove-etag.py \
275-Flcache-basic1.py \
276-Flcache-cookie1.py \
277-Flcache-cookie2.py \
278-Flcache-headers1.py \
279-Flcache-headers2.py \
280-Flcache-gzip1.py \
281-Flcache-expired1.py \
282-Flcache-expired2.py \
283-Flcache-expired3.py \
284-SSI-include-recursive.py \
285-Flcache-overwrite.py \
286-CGI-Permissions.py \
287-rule-inheritance.py \
288-GZip-IE16.py \
289-Connection_TE.py \
290-Question-mark-in-name.py \
291-Redir-keepalive.py \
292-HSTS1.py \
293-HSTS-subdomains1.py \
294-HSTS-subdomains2.py \
295-custom-error-cgi.py \
296-EVHost4.py \
297-EVHost5.py \
298-Timeout.py \
299-MethodsRequestBodyHandling.py \
300-xsendfile3.py \
301-XForwardedFor-4.py \
302-DirIndex3.py \
304-Dirlist-TransferEncoding.py \
305-Error-ContentLength.py \
306-NoContent-keepalive.py \
307-ServerInfo.py \
308-HandlerErrorXSS.py \
309-CGIHeaders.py \
310-Authorization-empty.py \
311-Authorization-empty.py \
312-If-Range3.py \
313-If-Range4.py \
314-If-Range5.py

if USE_OPENSSL
ssl-keys/set-1.pem: ssl-keys/set-1.key openssl.cnf
	@OPENSSL_BIN@ req -batch -config openssl.cnf -new -x509 -key ssl-keys/set-1.key -out ssl-keys/set-1.pem -days 1095
ssl-keys/set-1.key:
	mkdir -p ssl-keys
	@OPENSSL_BIN@ genrsa -out ssl-keys/set-1.key 2048
test-ssl: ssl-keys/set-1.pem
	$(PYTHON) ./run-tests.py -s
test: test-non-ssl test-ssl

DISTCLEANFILES += ssl-keys/set-1.pem ssl-keys/set-1.key

else
test: test-non-ssl
endif

test-non-ssl:
	$(PYTHON) -m compileall .
	$(PYTHON) ./run-tests.py

clean-local:
	find . -type f -name "*.pyc" -exec rm -f {} \;
